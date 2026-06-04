#define F_CPU 1000000UL // 1MHz internal oscillator clock speed
#include <avr/io.h>
#include <util/delay.h>

// --- NEW 1-WIRE PIN CONFIGURATION ---
// Moved to PB0 to avoid colliding with the display segments on PORTD!
#define ONEWIRE_PIN  PB0
#define ONEWIRE_DDR  DDRB
#define ONEWIRE_PORT PORTB
#define ONEWIRE_IN   PINB

#define DIG0 PC0
#define DIG1 PC1
#define DIG2 PC2
#define DIG3 PC3

// Digits array for the 4 multiplexed screens
uint8_t digits[4] = {10, 10, 10, 10}; // Start completely blank

// Custom Hex Array based on your Capacitance Meter (A=Bit7, B=Bit6... DP=Bit0)
// Indices 0-9 are numbers. 
// Index 10 is Blank. Index 11 is the Degree symbol '°'. Index 12 is 'C'.
const uint8_t ssd_digits[] = {
    0xFC, 0x60, 0xDA, 0xF2, 0x66, 0xB6, 0xBE, 0xE0, 0xFE, 0xF6, 
    0x00, 0xC6, 0x9C 
};

// --- 1-Wire Protocol Functions (Now mapped to PB0) ---

uint8_t onewire_reset(void) {
    uint8_t presence = 0;
    ONEWIRE_DDR |= (1 << ONEWIRE_PIN);  
    ONEWIRE_PORT &= ~(1 << ONEWIRE_PIN);
    _delay_us(480);                      
    
    ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);  
    _delay_us(60); 
    
    if (!(ONEWIRE_IN & (1 << ONEWIRE_PIN))) {
        presence = 1; 
    }
    _delay_us(420);                      
    return presence;
}

void onewire_write_bit(uint8_t bit) {
    ONEWIRE_DDR |= (1 << ONEWIRE_PIN);  
    ONEWIRE_PORT &= ~(1 << ONEWIRE_PIN);
    asm volatile("nop"); asm volatile("nop");
    
    if (bit) ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN); 
    _delay_us(55); 
    ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);  
    asm volatile("nop");
}

uint8_t onewire_read_bit(void) {
    uint8_t bit = 0;
    ONEWIRE_DDR |= (1 << ONEWIRE_PIN);  
    ONEWIRE_PORT &= ~(1 << ONEWIRE_PIN);
    asm volatile("nop"); 
    
    ONEWIRE_DDR &= ~(1 << ONEWIRE_PIN);  
    asm volatile("nop"); asm volatile("nop");
    
    if (ONEWIRE_IN & (1 << ONEWIRE_PIN)) bit = 1;
    _delay_us(50);                      
    return bit;
}

void onewire_write_byte(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        onewire_write_bit(data & 0x01);
        data >>= 1;
    }
}

uint8_t onewire_read_byte(void) {
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (onewire_read_bit()) data |= (1 << i);
    }
    return data;
}

// --- Exact Display Logic from your Capacitance Meter ---

void setSegments(uint8_t seg_data) {
    PORTD = (PORTD & 0xC0) | (seg_data & 0x3F);

    if(seg_data & 0x40) PORTC |= (1 << PC4);
    else PORTC &= ~(1 << PC4);

    if(seg_data & 0x80) PORTC |= (1 << PC5);
    else PORTC &= ~(1 << PC5);
}

void ssdDisplay(void) {
    for(uint8_t i = 0; i < 4; i++) {
        // Turn off all digits
        PORTC &= ~((1<<DIG0) | (1<<DIG1) | (1<<DIG2) | (1<<DIG3));

        uint8_t seg_data = ssd_digits[digits[i]];
        setSegments(seg_data);

        // Turn on the specific digit transistor
        if(i == 0) PORTC |= (1 << DIG0);
        if(i == 1) PORTC |= (1 << DIG1);
        if(i == 2) PORTC |= (1 << DIG2);
        if(i == 3) PORTC |= (1 << DIG3);

        _delay_ms(2); // 2ms multiplexing delay
    }
    // Clean up
    PORTC &= ~((1<<DIG0) | (1<<DIG1) | (1<<DIG2) | (1<<DIG3));
}

// --- Main Program ---

int main(void) {
    // Hardware Init based on your Cap Meter specs
    DDRC |= (1 << DIG0) | (1 << DIG1) | (1 << DIG2) | (1 << DIG3); // Digit Transistors
    PORTC &= ~((1 << DIG0) | (1 << DIG1) | (1 << DIG2) | (1 << DIG3));
    
    DDRD |= 0x3F; // Segments PD0-PD5
    DDRC |= (1 << PC4) | (1 << PC5); // Segments PC4, PC5
    
    int8_t temp = 0;
    uint8_t lsb, msb;
    int16_t raw_temp;
    
    while (1) {
        if (onewire_reset()) {
            onewire_write_byte(0xCC); // Skip ROM
            onewire_write_byte(0x44); // Start Temperature Conversion
            
            // ACTIVE DELAY: Multiplex the display while sensor thinks!
            // ssdDisplay() takes ~8ms to run. 95 loops * 8ms = ~760ms!
            for (uint8_t i = 0; i < 95; i++) {
                ssdDisplay();
            }
            
            if (onewire_reset()) {
                onewire_write_byte(0xCC); // Skip ROM
                onewire_write_byte(0xBE); // Read Scratchpad
                
                lsb = onewire_read_byte();
                msb = onewire_read_byte();
                
                raw_temp = (msb << 8) | lsb;
                temp = (int8_t)(raw_temp / 16); 
                
                // Format for 4-digit display: e.g., " 24°C "
                if (temp > 99) temp = 99;
                if (temp < 0)  temp = 0; 
                
                digits[0] = temp / 10;   // Tens digit
                if (digits[0] == 0) digits[0] = 10; // Hide leading zero (make it blank)
                
                digits[1] = temp % 10;   // Ones digit
                digits[2] = 11;          // The Degree symbol '°'
                digits[3] = 12;          // The letter 'C'
            }
        } else {
            // ERROR: Sensor unplugged. Display " E r"
            digits[0] = 10; // Blank
            digits[1] = 14; // We don't have an 'E' defined, so let's just make it blank for now
            digits[2] = 10; 
            digits[3] = 10; 
            ssdDisplay();
        }
    }
}

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
// #include "maxHeap.h"
volatile uint8_t portdhistory = 0xFF;

// ├── Funcionamento
//     |  ├── Atende andares de CIMA para BAIXO ("MAX HEAP")
//     |  ├── Todo mundo só pode descer para o 1° andar
//     |  ├── Quando estiver parado volta para o 1° andar
//     |  ├── Ex: se ele é chamado no 2° e depois no 4°, primeiro passa no 4°, 2°, 1°

// ├── Elevador possui:
//     |  ├── botão em cada andar
//     |  ├── led em cada andar


/*
-A lógica de movimento do elevador é a seguinte:
-O elevador inicia no primeiro andar.
-Ao ser chamado, ele se desloca para o andar especificado (o deslocamento deve ser apresentado visualmente no LCD, e deve demorar alguns segundos).
-Se outro(s) botão(s) for pressionado durante o movimento, o elevador "lembra" dos andares que solicitaram parada.
-O elevador atende os andares de CIMA para BAIXO, isto é, ele sempre tenta ir para o último andar solicitado e depois vai descendo. Se durante a descida ele for novamente chamado por um andar mais alto,
só atenderá depois de chegar no andar mais baixo chamado.
-Após alguém "entrar" no elevador (isto é, elevador chegou num andar onde foi chamado), essa pessoa "aperta" o botão do 1o. andar depois de 3 segundos.

*/
// led acende quando o elevador chega no andar esperado

// ------------------------------------------------------------------------------------------------

//#define F_CPU 20000000

int fila_atual[] = {0, 0, 0, 0};
int fila_espera[] = {0, 0, 0, 0};
int pos = 0;
int posEsp = 0;
int estado;
int maiorAndar;
int auxTime;

/*void USART_Init(void)
{
    // Seta taxa de transmissão/recepção (baud rate)
    UBRR0H = (uint8_t)(USART_UBBR_VALUE >> 8);
    UBRR0L = (uint8_t)USART_UBBR_VALUE;     
    // Seta formato do frame de transmissão: 8 bits de dados, sem paridade, 1 stop bit
    UCSR0C = (0 << USBS0) | (3 << UCSZ00);
    // Habilita receptor e transmissor
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
}*/

// void USART_Puts(char str[])
// {
//     // Espera se um byte  estiver sendo transmitido
//     while ((UCSR0A & (1 << UDRE0)) == 0);

//     int i = 0;
//     while (str[i])
//     {
//         UDR0 = str[i];
//         i++;
//     }
// }

void swap(int *a, int *b);
int procura(int fila[], int num);
void heapify(int arr[], int n, int i);
void dequeue(int fila[]);
void buildHeap(int arr[], int n);
void nokia_lcd_init(void);
void nokia_lcd_clear(void);
void nokia_lcd_set_cursor(uint8_t x, uint8_t y);
void nokia_lcd_write_string(const char *str, uint8_t scale);
void nokia_lcd_render(void);

ISR(PCINT2_vect)
{
    uint8_t changedbits;

    changedbits = PIND ^ portdhistory;
    portdhistory = PIND;

    if(estado == 2){estado = 0;}
    if (changedbits & (1 << PD1))
    {   
        if(procura(fila_espera, 1)==0 && estado == 1 && fila_atual[0] < 1){
            fila_espera[posEsp] = 1;
            posEsp++;
        }
        else if(procura(fila_atual, 1)==0 && procura(fila_espera, 1)==0){
            fila_atual[pos] = 1;
            pos++;
        }
    }


    else if (changedbits & (1 << PD2))
    {
        if(procura(fila_espera, 2)==0 && estado == 1 && fila_atual[0] < 2){
            fila_espera[posEsp] = 2;
            posEsp++;
        }
        else if(procura(fila_atual, 2)==0 && procura(fila_espera, 2)==0){
            fila_atual[pos] = 2;
            pos++;
        }
    }



    else if (changedbits & (1 << PD3))
    {
        if(procura(fila_espera, 3)==0 && estado == 1 && fila_atual[0] < 3){
            fila_espera[posEsp] = 3;
            posEsp++;
        }
        else if(procura(fila_atual, 3)==0 && procura(fila_espera, 3)==0){
            fila_atual[pos] = 3;
            pos++;
        }
    }



    else if (changedbits & (1 << PD4))
    {
        if(procura(fila_espera, 4)==0 && estado == 1 && fila_atual[0] < 4){
            fila_espera[posEsp] = 4;
            posEsp++;
        }
        else if(procura(fila_atual, 4)==0 && procura(fila_espera, 4)==0){
            fila_atual[pos] = 4;
            pos++;
        }
    }

    int n = sizeof(fila_atual) / sizeof(fila_atual[0]);
    buildHeap(fila_atual, n);

}

// ISR(TIMER1_COMPA_vect){
//     //estado = 1;
//     auxTime++;
//     if(auxTime == 3){
//         PORTB ^= (1 << PORTB0);
//         TCNT1 = 23437;
//         auxTime = 0;
//     }  
// }

int main()
{
    
    nokia_lcd_init();
    nokia_lcd_clear();
    nokia_lcd_set_cursor(36, 15);
    nokia_lcd_write_string("1\001", 2);
    nokia_lcd_render();
    
    estado = 2;
    maiorAndar = 0;

    // Definindo as saídas
    DDRC |= (1 << PC1) | (1 << PC2) | (1 << PC3) | (1 << PC4);
    DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB4);

    // Definindo as entradas
    DDRD &= (0 << PD1) & (0 << PD2) & (0 << PD3) & (0 << PD4);

    // Definindo as interrupções
    PCICR |= (1 << PCIE2);                                                       // habilita vetor de interrupção de todos os PDs
    PCMSK2 |= (1 << PCINT17) | (1 << PCINT18) | (1 << PCINT19) | (1 << PCINT20); // habilita interrupção de PD1 à PD4

    // Habilitando o pull-up (sempre passa 1)
    PORTD |= (1 << PD1) | (1 << PD2) | (1 << PD3) | (1 << PD4);

    //TCCR1B = (1 << WGM12);

    //OCR1A = 19531;

    //TIMSK1 = (1 << OCIE1A);
    
    sei();
    
    //TCCR1B |= (1 << CS12) | (1 << CS10);
    while (1)
    {   
        if(estado == 0){
            _delay_ms(500);
            _delay_ms(500);
            _delay_ms(500);
            _delay_ms(500);
            if (fila_atual[0] == 1)
            {
                nokia_lcd_init();
                nokia_lcd_clear();
                nokia_lcd_set_cursor(36, 15);
                nokia_lcd_write_string("1\001", 2);
                nokia_lcd_render();
                dequeue(fila_atual);
                int n = sizeof(fila_atual) / sizeof(fila_atual[0]);
                buildHeap(fila_atual, n);
                // _delay_ms(2000);
                PORTC = (1 << PC1);
                _delay_ms(3000);
                // nokia_lcd_clear();
                // nokia_lcd_render();
                PORTC = 0;
                estado = 1;
            }
            else if(fila_atual[0] > 1){
                nokia_lcd_init();
                nokia_lcd_clear();
                nokia_lcd_set_cursor(36, 15);
                nokia_lcd_write_string("1\001", 2);
                nokia_lcd_render();
                _delay_ms(1500);
                nokia_lcd_clear();
                nokia_lcd_render();
            }
            
            

            if (fila_atual[0] == 2)
            {
                nokia_lcd_init();
                nokia_lcd_clear();
                nokia_lcd_set_cursor(36, 15);
                nokia_lcd_write_string("2\001", 2);
                nokia_lcd_render();
                dequeue(fila_atual);
                int n = sizeof(fila_atual) / sizeof(fila_atual[0]);
                buildHeap(fila_atual, n);
                // _delay_ms(2000);
                PORTC = (1 << PC2);
                _delay_ms(3000);
                nokia_lcd_clear();
                PORTC = 0;
                estado = 1;
                maiorAndar = 2;
            }
            else if(fila_atual[0] > 2){
                nokia_lcd_init();
                nokia_lcd_clear();
                nokia_lcd_set_cursor(36, 15);
                nokia_lcd_write_string("2\001", 2);
                nokia_lcd_render();
                _delay_ms(1500); 
                nokia_lcd_clear();
                nokia_lcd_render();
            }


            if (fila_atual[0] == 3)
            {
                nokia_lcd_init();
                nokia_lcd_clear();
                nokia_lcd_set_cursor(36, 15);
                nokia_lcd_write_string("3\001", 2);
                nokia_lcd_render();
                dequeue(fila_atual);
                int n = sizeof(fila_atual) / sizeof(fila_atual[0]);
                buildHeap(fila_atual, n);
                // _delay_ms(2000);
                PORTC = (1 << PC3);
                _delay_ms(3000);
                nokia_lcd_clear();
                nokia_lcd_render();
                PORTC = 0;
                estado = 1;
                maiorAndar = 3;
            }
            else if(fila_atual[0] > 3){
                nokia_lcd_init();
                nokia_lcd_clear();
                nokia_lcd_set_cursor(36, 15);
                nokia_lcd_write_string("3\001", 2);
                nokia_lcd_render();
                _delay_ms(1500); 
                nokia_lcd_clear();
                nokia_lcd_render();
            }



            if (fila_atual[0] == 4)
            {
                nokia_lcd_init();
                nokia_lcd_clear();
                nokia_lcd_set_cursor(36, 15);
                nokia_lcd_write_string("4\001", 2);
                nokia_lcd_render();
                dequeue(fila_atual);
                int n = sizeof(fila_atual) / sizeof(fila_atual[0]);
                buildHeap(fila_atual, n);
                // _delay_ms(2000);
                PORTC = (1 << PC4);
                _delay_ms(3000);
                nokia_lcd_clear();
                nokia_lcd_render();
                PORTC = 0;
                estado = 1;
                maiorAndar = 4;
            }
        }
        //DESCIDA
        if(estado == 1){

            if (fila_atual[0] == 3)
            {
                nokia_lcd_init();
                nokia_lcd_clear();
                nokia_lcd_set_cursor(36, 15);
                nokia_lcd_write_string("3\001", 2);
                nokia_lcd_render();
                dequeue(fila_atual);
                int n = sizeof(fila_atual) / sizeof(fila_atual[0]);
                buildHeap(fila_atual, n);
                // _delay_ms(2000);
                PORTC = (1 << PC3);
                _delay_ms(2000);
                nokia_lcd_clear();
                nokia_lcd_render();
                PORTC = 0;
            }
            else if(maiorAndar > 3){
                nokia_lcd_init();
                nokia_lcd_clear();
                nokia_lcd_set_cursor(36, 15);
                nokia_lcd_write_string("3\001", 2);
                nokia_lcd_render();
                _delay_ms(1500);
                nokia_lcd_clear();
                nokia_lcd_render();
            }

            if (fila_atual[0] == 2)
            {   
                nokia_lcd_init();
                nokia_lcd_clear();
                nokia_lcd_set_cursor(36, 15);
                nokia_lcd_write_string("2\001", 2);
                nokia_lcd_render();
                dequeue(fila_atual);
                int n = sizeof(fila_atual) / sizeof(fila_atual[0]);
                buildHeap(fila_atual, n);
                // _delay_ms(2000);
                PORTC = (1 << PC2);
                _delay_ms(2000);
                nokia_lcd_clear();
                nokia_lcd_render();
                PORTC = 0;
            }
            else if(maiorAndar > 2){
                nokia_lcd_init();
                nokia_lcd_clear();
                nokia_lcd_set_cursor(36, 15);
                nokia_lcd_write_string("2\001", 2);
                nokia_lcd_render();
                _delay_ms(1500);
                nokia_lcd_clear();
                nokia_lcd_render();
            }

            if (fila_atual[0] == 1)
            {
                nokia_lcd_init();
                nokia_lcd_clear();
                nokia_lcd_set_cursor(36, 15);
                nokia_lcd_write_string("1\001", 2);
                nokia_lcd_render();
                dequeue(fila_atual);
                int n = sizeof(fila_atual) / sizeof(fila_atual[0]);
                buildHeap(fila_atual, n);
                // _delay_ms(2000);
                PORTC = (1 << PC1);
                _delay_ms(2000);
                PORTC = 0;
            }
            else if(maiorAndar > 1){
                nokia_lcd_init();
                nokia_lcd_clear();
                nokia_lcd_set_cursor(36, 15);
                nokia_lcd_write_string("1\001", 2);
                nokia_lcd_render();
                _delay_ms(1500);
            }
        
            for(int i = 0; i < 4; i++){
                fila_atual[i] = fila_espera[i];
                fila_espera[i] = 0;
            }
            posEsp = 0;
            int n = sizeof(fila_atual) / sizeof(fila_atual[0]);
            buildHeap(fila_atual, n);

            if(fila_atual[0] == 0)
            {
                estado = 2;
            }
            else{
                estado = 0;
            }

            for(int i = 0; i < 4; i++){
                if(fila_atual[i] == 0){
                    pos = i;
                    break;
                }
                else{
                    pos = 4;
                }
            }
            
            maiorAndar = 0;
        }
    }
}


int procura(int fila[], int num){
    int veri = 0;
    for(int i = 0; i < 4; i++){
        if(fila[i] == num){
            veri = 1;
        }
    }
    return veri;
}

void dequeue(int fila[]){
    fila[0] = fila[1];
    fila[1] = fila[2];
    fila[2] = fila[3];
    fila[3] = 0;
    pos--;
}

// --------------------------------------------------------------------------------

void swap(int *a, int *b)
{
    int temp = *b;
    *b = *a;
    *a = temp;
}

// Function to print the Heap as array

// will print as - 'message array[]\n'

// Size of heap is n

void heapify(int arr[], int n, int i)
{
    int largest = i;            // Initialize largest as root
    int leftChild = 2 * i + 1;  // left child = 2*i + 1
    int rightChild = 2 * i + 2; // right child = 2*i + 2

    // If left child is greater than root

    if (leftChild < n && arr[leftChild] > arr[largest])
        largest = leftChild;

    // If right child is greater than new largest

    if (rightChild < n && arr[rightChild] > arr[largest])
        largest = rightChild;

    // If largest is not the root

    if (largest != i)
    {
        // swap root with the new largest

        swap(&arr[i], &arr[largest]);

        // Recursively heapify the affected sub-tree i.e, subtree with root as largest
        heapify(arr, n, largest);
    }
}

// Function to build a Max-Heap from a given array

void buildHeap(int arr[], int n)
{
    // Index of last non-leaf node
    int lastNonLeafNode = (n / 2) - 1;

    // Perform level order traversal in reverse from last non-leaf node to the root node and heapify each node
    for (int i = lastNonLeafNode; i >= 0; i--)
    {
        heapify(arr, n, i);
    }
}

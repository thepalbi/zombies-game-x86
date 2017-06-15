; ** por compatibilidad se omiten tildes **
; ==============================================================================
; TRABAJO PRACTICO 3 - System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
; ==============================================================================
; definicion de rutinas de atencion de interrupciones

%include "imprimir.mac"

BITS 32

sched_tarea_offset:     dd 0x00
sched_tarea_selector:   dw 0x00

interrupt_msg db     'U FUCKED UP MATE, CHECK EAX'
interrupt_msg_len equ    $ - interrupt_msg

;; PIC
extern fin_intr_pic1

;; Sched
extern sched_proximo_indice
extern print_int_error
extern print_hex

extern areloco

;;
;; Definición de MACROS
;; -------------------------------------------------------------------------- ;;

%macro ISR 1
global _isr%1

_isr%1:
    mov eax, %1
    xchg bx, bx
    ;imprimir_texto_mp interrupt_msg, interrupt_msg_len, 0x04, 25, 40 - interrupt_msg_len/2
    call print_int_error
    jmp $

%endmacro

;;
;; Datos
;; -------------------------------------------------------------------------- ;;
; Scheduler
isrnumero:           dd 0x00000000
isrClock:            db '|/-\'

;;
;; Rutina de atención de las EXCEPCIONES
;; -------------------------------------------------------------------------- ;;
divide_error_msg db     'Divide Error'
divide_error_len equ    $ - divide_error_msg
global _isr0
_isr0:
    imprimir_texto_mp divide_error_msg, divide_error_len, 0x07, 0, 0
    jmp $

overflow_msg db     'Overflow'
overflow_len equ    $ - overflow_msg
global _isr4
_isr4:
    imprimir_texto_mp overflow_msg, overflow_len, 0x07, 0, 0
    jmp $

range_msg db     'BOUND Range Exceeded'
range_len equ    $ - overflow_msg
global _isr5
_isr5:
    imprimir_texto_mp range_msg, range_len, 0x07, 0, 0
    jmp $

opcode_msg db     'Invalid Opcode'
opcode_len equ    $ - opcode_msg
global _isr6
_isr6:
    imprimir_texto_mp opcode_msg, opcode_len, 0x07, 0, 0
    jmp $

not_present_msg db     'Segment Not Present'
not_present_len equ    $ - not_present_msg
global _isr11
_isr11:
    imprimir_texto_mp not_present_msg, not_present_len, 0x07, 0, 0
    jmp $

protection_msg db     'General Protection'
protection_len equ    $ - protection_msg
global _isr13
_isr13:
    imprimir_texto_mp protection_msg, protection_len, 0x07, 0, 0
    jmp $

ISR 1
ISR 2
ISR 3
ISR 7
ISR 8
ISR 9
ISR 10
ISR 12
ISR 14
ISR 15
ISR 16
ISR 17
ISR 18
ISR 19

;;
;; Rutina de atención del RELOJ
;; -------------------------------------------------------------------------- ;;
%define LAST_POXEL 80*49*2+79*2
%define CLOCK_BG 0x0F
; POXEL CLOCK: ah -> [ ATTR | CHAR ] <- al
global _isr32
_isr32:
  push eax
  push ebx
  xor ebx, ebx
  mov ah, CLOCK_BG ; Cargo el fondo de poxel del clock
  mov bl, [.last_char] ; Mer cargo el ultimos index el clock
  cmp bl, 0x8
  je .reset_clock
  mov al, [.clock_char + ebx]
  mov word [es:LAST_POXEL], ax
  inc bl
  mov [.last_char], bl
  jmp .fin_clock
.reset_clock:
  mov byte[.last_char], 0x1
  mov al, 0x7C
  mov word [es:LAST_POXEL], ax
.fin_clock:
  call fin_intr_pic1
  pop ebx
  pop eax
  iret
.clock_char: DB 0x7C, 0x2F, 0x2D, 0x5C, 0x7C, 0x2F, 0x2D, 0x5C ; Secuencia de reloj para indexar
.last_char: DB 0x0 ; Ultimo index -> Resetea en 8

;;
;; Rutina de atención del TECLADO
;; -------------------------------------------------------------------------- ;;
global _isr33
_isr33:
push eax

push 0x24
push 10
push 10
push 2
xor eax, eax
in al, 0x60
push eax
call print_hex
pop eax
pop eax
pop eax
pop eax
pop eax

call fin_intr_pic1
pop eax
iret

;;
;; Rutinas de atención de las SYSCALLS
;; -------------------------------------------------------------------------- ;;

%define IZQ 0xAAA
%define DER 0x441
%define ADE 0x83D
%define ATR 0x732


;; Funciones Auxiliares
;; -------------------------------------------------------------------------- ;;
proximo_reloj:
        pushad
        inc DWORD [isrnumero]
        mov ebx, [isrnumero]
        cmp ebx, 0x4
        jl .ok
                mov DWORD [isrnumero], 0x0
                mov ebx, 0
        .ok:
                add ebx, isrClock
                imprimir_texto_mp ebx, 1, 0x0f, 49, 79
                popad
        ret
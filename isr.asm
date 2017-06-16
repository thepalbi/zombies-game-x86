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
extern sched_proximo_indice

extern areloco
;;
;; Definición de MACROS
;; -------------------------------------------------------------------------- ;;

%macro ISR 1
global _isr%1

_isr%1:
    mov eax, %1
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
ISR 0
ISR 1
ISR 2
ISR 3
ISR 4
ISR 5
ISR 6
ISR 7
ISR 8
ISR 9
ISR 10
ISR 11
ISR 12
ISR 13
ISR 14
ISR 15
ISR 16
ISR 17
ISR 18
ISR 19
ISR 20
;;
;; Rutina de atención del RELOJ
;; -------------------------------------------------------------------------- ;;
%define LAST_POXEL 80*49*2+79*2
%define CLOCK_BG 0x0F
%define GDT_IDX_TSS_PRIMERO 13
; POXEL CLOCK: ah -> [ ATTR | CHAR ] <- al
global _isr32
_isr32:
  ; Actualizando relojito
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

  call sched_proximo_indice
  add eax, GDT_IDX_TSS_PRIMERO
  shl eax, 3
  mov [jump_far_selector], ax
  ; TODO: preguntar si la task no es la misma en la que estoy. explota todo porque estaría busy
  jmp far [jump_far_address]


  iret
.clock_char: DB 0x7C, 0x2F, 0x2D, 0x5C, 0x7C, 0x2F, 0x2D, 0x5C ; Secuencia de reloj para indexar
.last_char: DB 0x0 ; Ultimo index -> Resetea en 8
.jump_far_address:  DD 0xBEBECACA ; fruta que nos chupa dos huevos
.jump_far_selector: DW 0xDEAD ; fruta que vamos a sobreescribir
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

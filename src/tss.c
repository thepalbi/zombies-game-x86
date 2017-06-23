/* ** por compatibilidad se omiten tildes **
================================================================================
 TRABAJO PRACTICO 3 - System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
================================================================================
  definicion de estructuras para administrar tareas
*/

#include "defines.h"
#include "gdt.h"
#include "tss.h"
#include "mmu.h"


tss tss_inicial;
tss tss_idle;

tss tss_zombisB[CANT_ZOMBIS];
tss tss_zombisA[CANT_ZOMBIS];

unsigned int esp0B[CANT_ZOMBIS];
unsigned int esp0A[CANT_ZOMBIS];

unsigned int cr3B[CANT_ZOMBIS];
unsigned int cr3A[CANT_ZOMBIS];


// Bases en descriptores TSS en la GDT:
// Podemos estar seguros de que la base de las TSS van a estar
// en el kernel, AKA en las primeras 0x100 páginas, con lo cual
// lidiamos con direcciones de memoria menores a 0x100000, con
// lo cual nos bastan 21 bits, y entran en 'base'. Como en los descriptores las
// bases están fragmentadas, defino las siguientes macros:
#define BITS_0_15(base) (unsigned int)base & 0xFFFF // base[0:15]
#define BITS_16_23(base) ((unsigned int)base >> 16) & 0xFF  // base[23:16]


// INICIALIZACIÓN
// --------------

void tss_inicializar() {
      // Inicialización de TSS y Task Gates
      tss_inicializar_idle();
      tss_inicializar_inicial();
      int i;
      for (i = 0; i < CANT_ZOMBIS; i++) {
            tss_inicializar_zombi(player_A, i);
            tss_inicializar_zombi(player_B, i);
      }
      // Storage de variables globales
      for (i = 0; i < CANT_ZOMBIS; i++) {
            cr3A[i] = tss_zombisA[i].cr3;
            cr3B[i] = tss_zombisB[i].cr3;
            esp0A[i] = tss_zombisA[i].esp0;
            esp0B[i] = tss_zombisB[i].esp0;
      }
}

void tss_inicializar_idle() {
      // TSS
      tss_idle = (tss) {
            .esp0 = DIR_INICIO_PILA_KERNEL,
            .ss0 = GDT_DESC_DATA_KERNEL,
            .cr3 = 0x27000,
            .eip = DIR_INICIO_TASK_IDLE,
            .esp = DIR_INICIO_PILA_KERNEL,
            .ebp = DIR_INICIO_PILA_KERNEL,
            .eflags = 0x202, // Seteamos IF y Reserved
            .fs = GDT_DESC_SCREEN,
            .cs = GDT_DESC_CODE_KERNEL,
            .ss = GDT_DESC_DATA_KERNEL,
            .ds = GDT_DESC_DATA_KERNEL,
            .es = GDT_DESC_DATA_KERNEL,
            .gs = GDT_DESC_DATA_KERNEL
      };

      // Task Gate
      gdt[GDT_IDX_TSS_IDLE] = (gdt_entry){
            (unsigned short)    TSS_SIZE-1,            /* limit[0:15]   */
            (unsigned short)    BITS_0_15(&tss_idle),  /* base[0:15]    */
            (unsigned char)     BITS_16_23(&tss_idle), /* base[23:16]   */
            (unsigned char)     0x9,                   /* type (tss)    */
            (unsigned char)     0x0,                   /* s (supervisor)*/
            (unsigned char)     0x00,                  /* dpl           */
            (unsigned char)     0x01,                  /* p             */
            (unsigned char)     0x00,                  /* limit[16:19]  */
            (unsigned char)     0x00,                  /* avl           */
            (unsigned char)     0x00,                  /* l             */
            (unsigned char)     0x01,                  /* db            */
            (unsigned char)     0x00,                  /* g             */
            (unsigned char)     0x00,                  /* base[31:24]   */
      };
}

void tss_inicializar_inicial() {
      // Si bien no hace falta que la TSS tenga un contexto posta,
      // el selector en la GDT tiene que estar bien (tr)
      gdt[GDT_IDX_TSS_INIT] = (gdt_entry){
            (unsigned short)    TSS_SIZE-1,            /* limit[0:15]   */
            (unsigned short)    BITS_0_15(&tss_inicial),  /* base[0:15]    */
            (unsigned char)     BITS_16_23(&tss_inicial), /* base[23:16]   */
            (unsigned char)     0x9,                   /* type (tss)    */
            (unsigned char)     0x0,                   /* s (supervisor)*/
            (unsigned char)     0x00,                  /* dpl           */
            (unsigned char)     0x01,                  /* p             */
            (unsigned char)     0x00,                  /* limit[16:19]  */
            (unsigned char)     0x00,                  /* avl           */
            (unsigned char)     0x00,                  /* l             */
            (unsigned char)     0x01,                  /* db            */
            (unsigned char)     0x00,                  /* g             */
            (unsigned char)     0x00,                  /* base[31:24]   */
      };
}

void tss_inicializar_zombi(int jugador, unsigned int i) {
      // Memoria
      // Los cr3 van a tener mapeadas 0x800000 en adelante, pero no importa
      // , los zombis posta las van a pisar al crearse
      unsigned int cr3 = mmu_inicializar_esquema_zombi(player_A, 1);
      // Para el esp0 pedimos una página y apuntamos a la 'base'
      unsigned int esp0 = mmu_prox_pag_libre() + PAGE_SIZE;

      // TSS
      tss_completar_zombi(jugador, i, cr3, esp0);

      // Task Gate
      unsigned int task_index;
      tss* ptr_tss_zombi;
      if (jugador == player_A) {
            task_index = GDT_IDX_TSS_ZOMBIS_A + i;
            ptr_tss_zombi = tss_zombisA + i; // Puntero a tss_zombisA[i]
      } else {
            task_index = GDT_IDX_TSS_ZOMBIS_B + i;
            ptr_tss_zombi = tss_zombisB + i; // Puntero a tss_zombisB[i]
      }

      gdt[task_index] = (gdt_entry){
            (unsigned short)    TSS_SIZE-1,            /* limit[0:15]   */
            (unsigned short)    BITS_0_15(ptr_tss_zombi),  /* base[0:15]    */
            (unsigned char)     BITS_16_23(ptr_tss_zombi), /* base[23:16]   */
            (unsigned char)     0x9,                   /* type (tss)    */
            (unsigned char)     0x0,                   /* s (supervisor)*/
            (unsigned char)     0x00,                  /* dpl           */
            (unsigned char)     0x01,                  /* p             */
            (unsigned char)     0x00,                  /* limit[16:19]  */
            (unsigned char)     0x00,                  /* avl           */
            (unsigned char)     0x00,                  /* l             */
            (unsigned char)     0x01,                  /* db            */
            (unsigned char)     0x00,                  /* g             */
            (unsigned char)     0x00,                  /* base[31:24]   */
      };
}

void tss_completar_zombi(int jugador, unsigned int i, unsigned int cr3, unsigned int esp0) {
      // TSS
      tss* ptr_tss_zombi;
      if (jugador == player_A) {
            ptr_tss_zombi = tss_zombisA + i; // Puntero a tss_zombisA[i]
      } else {
            ptr_tss_zombi = tss_zombisB + i; // Puntero a tss_zombisB[i]
      }

      (*ptr_tss_zombi) = (tss) {
            .esp0 = esp0,
            .ss0 = GDT_DESC_DATA_KERNEL,
            .cr3 = cr3,
            .eip = DIR_INICIO_ZOMBI_VISION,
            .esp = DIR_INICIO_ZOMBI_PILA,
            .ebp = DIR_INICIO_ZOMBI_PILA,
            .eflags = 0x202, // Seteamos IF y Reserved
            .cs = GDT_DESC_CODE_USER, //Selector de GDT, bits de privilegio
            .ss = GDT_DESC_DATA_USER, //  es RPL, no DPL!
            .ds = GDT_DESC_DATA_USER,
            .es = GDT_DESC_DATA_USER,
            .fs = GDT_DESC_DATA_USER,
            .gs = GDT_DESC_DATA_USER,
      };
}


// INTERFAZ
// --------------

unsigned int tss_leer_cr3(unsigned int jugador, unsigned int i) {
      if (jugador == player_A) {
            return tss_zombisA[i].cr3;
      } else {
            return tss_zombisB[i].cr3;
      }
}

void tss_escribir_cr3(unsigned int jugador, unsigned int i, unsigned int cr3) {
      if (jugador == player_A) {
            tss_zombisA[i].cr3 = cr3;
      } else {
            tss_zombisB[i].cr3 = cr3;
      }
}

void tss_refrescar_zombi(int jugador, unsigned int i) {
      unsigned int cr3 =  jugador == player_A ? cr3A[i] : cr3B[i];
      unsigned int esp0 =  jugador == player_A ? esp0A[i] : esp0B[i];
      tss_completar_zombi(jugador, i, cr3, esp0);
}
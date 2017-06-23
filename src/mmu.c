/* ** por compatibilidad se omiten tildes **
================================================================================
 TRABAJO PRACTICO 3 - System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
================================================================================
  definicion de funciones del manejador de memoria
*/

#include "mmu.h"
#include "i386.h"
#include "tss.h"
#include "game.h"
#include "debug.h"

// INICIALIZACIÓN +++++++++++++++++++++++++++++++++++
// ++++++++++++++++++++++++++++++++++++++++++++++++++

void mmu_inicializar() {
	// Pag Libres Init
	prox_pag_lib = PRIMERA_PAG_LIBRE;
}

unsigned int mmu_inicializar_zombi(unsigned int tarea, int jugador, int y) {
    unsigned int cr3_zombi = mmu_inicializar_esquema_zombi(jugador, y);
    copiar_zombi(cr3_zombi, tarea, jugador);
    return cr3_zombi;
}

void mmu_inicializar_esquema_kernel(){
	// Index Init (porque c89)
	int i;

	// Kernel Page Directory (0x27000)
	pd_entry* pd = (pd_entry*) 0x27000;
	pd[0] = (pd_entry) {.p = 1, .rw = 1, .base = 0x28};
	for (i = 1; i < 1024; i++) {
		pd[i] = (pd_entry) { };
	}

	// Kernel Page Table (0x28000)
	pt_entry* pt = (pt_entry*) 0x28000;
	for (i = 0; i < 1024; i++) {
		pt[i] = (pt_entry) {.p = 1, .rw = 1, .base = i};
	}
}

unsigned int mmu_inicializar_esquema_zombi(int jugador, int y) {
    // Index Declaration
    int i;

    // Memory Allocation
    pd_entry* pd = (pd_entry*) mmu_prox_pag_libre();
    pt_entry* pt = (pt_entry*) mmu_prox_pag_libre();

    // Page Directory
    for (i = 0; i < 1024; i++) pd[i] = (pd_entry) { }; // zero everything
    pd[0] = (pd_entry) { .p = 1, .rw = 1, .us = 0, .base = 0x28 }; // first entry
    pd[0x20] = (pd_entry) { .p = 1, .rw = 1, .us = 1, .base = (unsigned int)pt>>12 }; // zombi sight table

    // Page Table
    for (i = 0; i < 1024; i++) pt[i] = (pt_entry) { }; // zero everything
    int x = jugador == player_A ? 0+1 : 77-1;
    mmu_mapear_vision_zombi(jugador, (unsigned int)pd, x, y);

    return (unsigned int) pd;
}


// MANEJO DE MEMORIA ++++++++++++++++++++++++++++++++
// ++++++++++++++++++++++++++++++++++++++++++++++++++

unsigned int mmu_prox_pag_libre(){
    unsigned int prev_page = prox_pag_lib;
    prox_pag_lib += PAGE_SIZE;
    return prev_page;
}

// Agregado parámetro 'user' para mapear tanto páginas de kernel (supervisor) como de zombi (user)
void mmu_mapear_pagina(unsigned int vir, unsigned int cr3, unsigned int fis, unsigned char user){
    if (((vir & 0xFFF) != 0) || ((fis & 0xFFF) != 0)) {
        error("ASSERT FAIL: dirs no multiplo de PAGESIZE (mmu_mapear_pagina)");
    }
    if (((prox_pag_lib & 0xFFF) != 0)) {
        error("ASSERT FAIL: proxpaglib no multiplo de PAGESIZE (mmu_mapear_pagina)");
    }

    pd_entry* pd = (pd_entry*) (cr3 & 0xFFFFF000);
    pt_entry* pt;

    pd_entry* ptr_pde = pd + INDEX_DIR(vir);
    if ((*ptr_pde).p == 0) {
        pt = (pt_entry*) mmu_prox_pag_libre();
        (*ptr_pde) = (pd_entry) { .p = 1, .rw = 1, .us = user, .base = (unsigned int) pt>>12 };
    } else {
        pt = (pt_entry*) ((*ptr_pde).base << 12);
    }

    pt[INDEX_TABLE(vir)] = (pt_entry) { .p = 1, .rw = 1, .us = user, .base = fis>>12 };
}

void mmu_unmapear_pagina(unsigned int vir, unsigned int cr3){
    if ((vir & 0xFFF) != 0) {
        error("ASSERT FAIL: vir no multiplo de PAGESIZE (mmu_unmapear_pagina)");
    }

    pd_entry* pd = (pd_entry*) (cr3 & 0xFFFFF000);
    pt_entry* pt = (pt_entry*) (pd[INDEX_DIR(vir)].base << 12);
    pt[INDEX_TABLE(vir)] = (pt_entry) { };
}

void mmu_mapear_vision_zombi(int jugador, unsigned int cr3, int x, int y){
    if (jugador != player_A && jugador != player_B) {
        error("ASSERT FAIL: jugador no valido (mmu_mapear_vision_zombi)");
    }

    if (jugador == player_A) {
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 0*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x, y), 1); // 1
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 1*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x+1, y), 1); // 2
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 2*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x+1, y+1), 1); // 3
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 3*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x+1, y-1), 1); // 4
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 4*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x, y+1), 1); // 5
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 5*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x, y-1), 1); // 6
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 6*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x-1, y), 1); // 7
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 7*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x-1, y-1), 1); // 8
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 8*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x-1, y+1), 1); // 9
    } else {
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 0*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x, y), 1); // 1
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 1*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x-1, y), 1); // 2
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 2*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x-1, y-1), 1); // 3
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 3*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x-1, y+1), 1); // 4
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 4*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x, y-1), 1); // 5
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 5*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x, y+1), 1); // 6
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 6*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x+1, y), 1); // 7
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 7*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x+1, y+1), 1); // 8
        mmu_mapear_pagina(DIR_INICIO_ZOMBI_VISION + 8*PAGE_SIZE, (unsigned int)cr3, dir_fisica(x+1, y-1), 1); // 9
    }
}


// AUXILIARES +++++++++++++++++++++++++++++++++++++++
// ++++++++++++++++++++++++++++++++++++++++++++++++++

unsigned int dir_fisica(int x, int y){
    int modulo_x = (x < 0) ? (x + MAP_WIDTH) % MAP_WIDTH : x % MAP_WIDTH; // Si x < 0 -> se le suma MAP_WIDTH para que pase a ser positivo, manteniendo su congruenecia, ya que MAP_WIDTH = 0 (MAP_WIDTH)
    int modulo_y = (y + MAP_HEIGHT) % MAP_HEIGHT; // idem modulo_x
    return DIR_INICIO_MAPA + (modulo_y * MAP_WIDTH + modulo_x)*PAGE_SIZE; // DIR_FISICA = (INICIO de las posiciones en mapa + cantidad de filas de arriba + posicion en x) * PAGE_SIZE
}

// Agregado cr3_zombi para que se encargue la función de hacer el cambió de cr3
// Recordar que el cambio de cr3 es porque el kernel no tiene mapeado el mapa, y
// para los zombies no es trivial obtener la página a donde quieren ir, con lo
// cual primero armamos/obtenemos el esquema de memoria para el zombi, y desde
// el mismo lo copiamos en el mapa físico
void copiar_zombi(unsigned int cr3_zombi, unsigned int task, int player){
    unsigned int cr3_original = rcr3();
    lcr3(cr3_zombi);
    tlbflush();

    // Tareas Jug 1 (A)
    // 1 0x10000 - 0x10FFF
    // 2 0x11000 - 0x11FFF
    // 3 0x12000 - 0x12FFF
    // Tareas Jug 2 (B)
    // 1 0x13000 - 0x13FFF
    // 2 0x14000 - 0x14FFF
    // 3 0x15000 - 0x15FFF

    // Las direccions de arriba están en la parte de kernel (identity mapping)
    char* dir_task;
    // base de jugador
    if(player==player_A){
        dir_task = (char*) 0x10000;
    }else{
        dir_task = (char*) 0x13000;
    }
    // offset de tarea
    dir_task += (task-1)*0x1000;

    char* dir_mapa = (char*) DIR_INICIO_ZOMBI_VISION;
    int i;
    for (i = 0; i < 0x1000; i++) {
        dir_mapa[i] = dir_task[i];
    }

    lcr3(cr3_original);
    tlbflush();
}

// Es parecido a copiar_zombi, pero para que usen los zombis. No podemos
// usar esa función porque te lo trae del kernel, con lo cual perdemos
// el estado que teniamos.
void replicar_zombi(direccion dir){
    char* dir_origen = (char*) DIR_INICIO_ZOMBI_VISION;
    char* dir_destino = (char*) DIR_INICIO_ZOMBI_VISION;
    if (dir == ADE) {
        dir_destino += 1*PAGE_SIZE; // pag #2
    } else if (dir == ATR) {
        dir_destino += 6*PAGE_SIZE; // pag #7
    } else if (dir == DER) {
        dir_destino += 4*PAGE_SIZE; // pag #5
    } else if (dir == IZQ) {
        dir_destino += 5*PAGE_SIZE; // pag #6
    }
    int i;
    for (i = 0; i < 0x1000; i++) {
        dir_destino[i] = dir_origen[i];
    }
}
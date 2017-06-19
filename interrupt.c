#include "interrupt.h"
#include "i386.h"
#include "screen.h"
#include "colors.h"
#include "mmu.h"
#include "sched.h"
#include "tss.h"
#include "game.h"
#include "debug.h"


void handle_kernel_exception(unsigned int code) {
	clear_line();
	char* str = "Generic Interruption";
	if (code == 0) {
		str = "FAULT: Divide Error";
	} else if (code == 6) {
		str = "FAULT: Invalid Opcode";
	} else if (code == 8) {
		str = "ABORT: Double Fault";
	} else if (code == 10) {
		str = "FAULT: Invalid TSS";
	} else if (code == 11) {
		str = "FAULT: Segment Not Present";
	} else if (code == 12) {
		str = "FAULT: Stack-Segment Fault";
	} else if (code == 13) {
		str = "FAULT: General Protection";
	} else if (code == 14) {
		str = "FAULT: Page Fault";
	}
	print_int(code, 1, 0, FG_LIGHT_RED | BG_BLACK);
	print(str, 3, 0, FG_LIGHT_RED | BG_BLACK);
	while (1) { hlt(); };
}

void handle_zombi_exception(unsigned int code) {
	// Matamos la tarea (la sacamos del scheduler)
	sched_desactivar_zombi(current_player(), current_task());

	// Borramos del mapa
	posicion pos;
	if (current_player() == player_A) {
		pos = zombis_pos_a[current_task()];
	} else {
		pos = zombis_pos_b[current_task()];
	}
	print(" ", pos.x+1, pos.y+1, BG_GREEN);

	// Refrescamos a los zombis
	print_zombis();

	// Dibujamos en la barra de control
	clear_line();
	if (current_player() == player_A) {
		print("X", 4+current_task()*2, 50-2, FG_RED | BG_BLACK);
	} else {
		print("X", 80-20+current_task()*2, 50-2, FG_RED | BG_BLACK);
	}

	// Let the people know
	print_int(code, 1, 0, FG_LIGHT_MAGENTA | BG_BLACK);
	print("Zombi", 3, 0, FG_LIGHT_MAGENTA | BG_BLACK);
	if (current_player() == player_A) {
		print("rojo", 9, 0, FG_LIGHT_MAGENTA | BG_BLACK);
	} else {
		print("azul", 9, 0, FG_LIGHT_MAGENTA | BG_BLACK);
	}
	print_int(current_task(), 14, 0, FG_LIGHT_MAGENTA | BG_BLACK);
	print("se recato", 16, 0, FG_LIGHT_MAGENTA | BG_BLACK);
}



void handle_keyboard(unsigned int key) {
	print_teclado(key);

	// Jugador A
	if (key == KEY_W) {
		game_mover_jugador(player_A, UP);
	} else if (key == KEY_S) {
		game_mover_jugador(player_A, DOWN);
	} else if (key == KEY_A) {
		game_cambiar_tipo(player_A, DOWN);
	} else if (key == KEY_D) {
		game_cambiar_tipo(player_A, UP);
	} else if (key == KEY_LS) {
		game_lanzar_zombi(player_A);
	}

	// Jugador B
	if (key == KEY_I) {
		game_mover_jugador(player_B, UP);
	} else if (key == KEY_K) {
		game_mover_jugador(player_B, DOWN);
	} else if (key == KEY_J) {
		game_cambiar_tipo(player_B, DOWN);
	} else if (key == KEY_L) {
		game_cambiar_tipo(player_B, UP);
	} else if (key == KEY_RS) {
		game_lanzar_zombi(player_B);
	}

	// Juego
	if (key == KEY_Y) {
		if (debug == false) {
			debug_on();
		} else {
			debug_off();
		}
		// TODO: este if también debería preguntar si el modo debug está
		// ON o OFF. Es otra varaible global a agregar
	}

}


void handle_syscall_mover(direccion d){
	//asm("xchg %bx, %bx");

	// Averiguar la posicion actual del zombie en la tarea
		// Averiguar pos_vieja y CR3
	unsigned int curr_player = current_player();
	unsigned int curr_task = current_task();
	posicion pos_vieja = (player_A == curr_player) ? zombis_pos_a[curr_task] : \
		zombis_pos_b[curr_task];
	unsigned int curr_cr3 = tss_leer_cr3(curr_player, curr_task);
	// Averiguar con la direccion para donde ir
		// Saber que direccion es la que debo ir
		// Calcular nueva posicion, asi el codigo siguiente es el mismo para cualquier opcion posible
	posicion destiny;
	int reverse = 1;
	if (curr_player == player_B) reverse = -1;
	if (d == IZQ) {
		destiny.x = pos_vieja.x;
		destiny.y = pos_vieja.y - (1 * reverse);
	}else if (d == DER) {
		destiny.x = pos_vieja.x;
		destiny.y = pos_vieja.y + (1 * reverse);
	}else if (d == ADE) {
		destiny.x = pos_vieja.x + (1*reverse);
		destiny.y = pos_vieja.y;
	}else{ // d==ATR
		destiny.x = pos_vieja.x - (1*reverse);
		destiny.y = pos_vieja.y;
	}
		// Actualizar posicion
		if (curr_player == player_A) {
			zombis_pos_a[curr_task] = destiny;
		}else{
			zombis_pos_b[curr_task] = destiny;
		}

		// 'Moverse'
		replicar_zombi(d);

		// Desmapear paginacion actual -> No me hace falta desmapear, solo piso el mapeo anterior
		// Mapearme las nuevas posiciones
		mmu_mapear_vision_zombi(curr_player, curr_cr3, destiny.x, destiny.y);

		// Borrarme del mapa mi iconito, dejando rastro
		print_zombie_trace(pos_vieja);

		// Refresco todos los zombis
		print_zombis();

	if ((destiny.x == 0 && curr_player == player_B) || \
		  (destiny.x == MAP_WIDTH-1 && curr_player == player_A)) {
		// Winning! ~ Charlie Sheen (1972-2017)
		// Acabo de buscar si murio, tranquilos chicos, Charlie still alive

		// Actualizamos info del juego
		if (destiny.x == 0) {
			puntaje_b++;
		} else {
			puntaje_a++;
		}

		// Matamos la tarea (la sacamos del scheduler)
		sched_desactivar_zombi(curr_player, curr_task);

		// Dibujamos en el mapa
		print(" ", destiny.x+1, destiny.y+1, BG_GREEN);

		// Dibujamos en la barra de control
		clear_line();
		if (curr_player == player_A) {
			print("X", 4+curr_task*2, 50-2, FG_RED | BG_BLACK);
		} else {
			print("X", 80-20+curr_task*2, 50-2, FG_RED | BG_BLACK);
		}
		print_puntajes(puntaje_a, puntaje_b);

		// Let the people know
		print("Zombi", 1, 0, FG_LIGHT_GREEN | BG_BLACK);
		if (current_player() == player_A) {
			print("rojo", 7, 0, FG_LIGHT_GREEN | BG_BLACK);
		} else {
			print("azul", 7, 0, FG_LIGHT_GREEN | BG_BLACK);
		}
		print_int(current_task(), 12, 0, FG_LIGHT_GREEN | BG_BLACK);
		print("se gano un bonobon", 14, 0, FG_LIGHT_GREEN | BG_BLACK);
	}


}

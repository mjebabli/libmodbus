/*
 * Copyright © 2001-2010 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <modbus.h>

/* The goal of this program is to check all major functions of
   libmodbus:
   - force_single_coil
   - read_coil_status
   - force_multiple_coils
   - preset_single_register
   - read_holding_registers
   - preset_multiple_registers
   - read_holding_registers

   All these functions are called with random values on a address
   range defined by the following defines.
*/
#define LOOP             1
#define MY_ID            1
#define SERVER_ID       17
#define ADDRESS_START    0
#define ADDRESS_END     99

/* At each loop, the program works in the range ADDRESS_START to
 * ADDRESS_END then ADDRESS_START + 1 to ADDRESS_END and so on.
 */
int main(void)
{
        int rc;
        int nb_fail;
        int nb_loop;
        int addr;
        int nb;
        uint8_t *tab_rq_status;
        uint8_t *tab_rp_status;
        uint16_t *tab_rq_registers;
        uint16_t *tab_rp_registers;
        modbus_param_t mb_param;

        /* RTU parity : none, even, odd */
        /*
          modbus_init_rtu(&mb_param, "/dev/ttyS0", 19200, "none", 8, 1, MY_ID);
        */

        /* TCP */
        modbus_init_tcp(&mb_param, "127.0.0.1", 1502);
        modbus_set_debug(&mb_param, TRUE);
        if (modbus_connect(&mb_param) == -1) {
                fprintf(stderr, "Connection failed: %s\n",
                        modbus_strerror(errno));
                return -1;
        }

        /* Allocate and initialize the different memory spaces */
        nb = ADDRESS_END - ADDRESS_START;

        tab_rq_status = (uint8_t *) malloc(nb * sizeof(uint8_t));
        memset(tab_rq_status, 0, nb * sizeof(uint8_t));

        tab_rp_status = (uint8_t *) malloc(nb * sizeof(uint8_t));
        memset(tab_rp_status, 0, nb * sizeof(uint8_t));

        tab_rq_registers = (uint16_t *) malloc(nb * sizeof(uint16_t));
        memset(tab_rq_registers, 0, nb * sizeof(uint16_t));

        tab_rp_registers = (uint16_t *) malloc(nb * sizeof(uint16_t));
        memset(tab_rp_registers, 0, nb * sizeof(uint16_t));

        nb_loop = nb_fail = 0;
        while (nb_loop++ < LOOP) {
                for (addr = ADDRESS_START; addr <= ADDRESS_END; addr++) {
                        int i;

                        /* Random numbers (short) */
                        for (i=0; i<nb; i++) {
                                tab_rq_registers[i] = (uint16_t) (65535.0*rand() / (RAND_MAX + 1.0));
                                tab_rq_status[i] = tab_rq_registers[i] % 2;
                        }
                        nb = ADDRESS_END - addr;

                        /* SINGLE COIL */
                        rc = force_single_coil(&mb_param, SERVER_ID, addr, tab_rq_status[0]);
                        if (rc != 1) {
                                printf("ERROR force_single_coil (%d)\n", rc);
                                printf("Slave = %d, address = %d, value = %d\n",
                                       SERVER_ID, addr, tab_rq_status[0]);
                                nb_fail++;
                        } else {
                                rc = read_coil_status(&mb_param, SERVER_ID, addr, 1, tab_rp_status);
                                if (rc != 1 || tab_rq_status[0] != tab_rp_status[0]) {
                                        printf("ERROR read_coil_status single (%d)\n", rc);
                                        printf("Slave = %d, address = %d\n",
                                               SERVER_ID, addr);
                                        nb_fail++;
                                }
                        }

                        /* MULTIPLE COILS */
                        rc = force_multiple_coils(&mb_param, SERVER_ID, addr, nb, tab_rq_status);
                        if (rc != nb) {
                                printf("ERROR force_multiple_coils (%d)\n", rc);
                                printf("Slave = %d, address = %d, nb = %d\n",
                                       SERVER_ID, addr, nb);
                                nb_fail++;
                        } else {
                                rc = read_coil_status(&mb_param, SERVER_ID, addr, nb, tab_rp_status);
                                if (rc != nb) {
                                        printf("ERROR read_coil_status\n");
                                        printf("Slave = %d, address = %d, nb = %d\n",
                                               SERVER_ID, addr, nb);
                                        nb_fail++;
                                } else {
                                        for (i=0; i<nb; i++) {
                                                if (tab_rp_status[i] != tab_rq_status[i]) {
                                                        printf("ERROR read_coil_status\n");
                                                        printf("Slave = %d, address = %d, value %d (0x%X) != %d (0x%X)\n",
                                                               SERVER_ID, addr,
                                                               tab_rq_status[i], tab_rq_status[i],
                                                               tab_rp_status[i], tab_rp_status[i]);
                                                        nb_fail++;
                                                }
                                        }
                                }
                        }

                        /* SINGLE REGISTER */
                        rc = preset_single_register(&mb_param, SERVER_ID, addr, tab_rq_registers[0]);
                        if (rc != 1) {
                                printf("ERROR preset_single_register (%d)\n", rc);
                                printf("Slave = %d, address = %d, value = %d (0x%X)\n",
                                       SERVER_ID, addr, tab_rq_registers[0], tab_rq_registers[0]);
                                nb_fail++;
                        } else {
                                rc = read_holding_registers(&mb_param, SERVER_ID, addr, 1, tab_rp_registers);
                                if (rc != 1) {
                                        printf("ERROR read_holding_registers single (%d)\n", rc);
                                        printf("Slave = %d, address = %d\n",
                                               SERVER_ID, addr);
                                        nb_fail++;
                                } else {
                                        if (tab_rq_registers[0] != tab_rp_registers[0]) {
                                                printf("ERROR read_holding_registers single\n");
                                                printf("Slave = %d, address = %d, value = %d (0x%X) != %d (0x%X)\n",
                                                       SERVER_ID, addr,
                                                       tab_rq_registers[0], tab_rq_registers[0],
                                                       tab_rp_registers[0], tab_rp_registers[0]);
                                                nb_fail++;
                                        }
                                }
                        }

                        /* MULTIPLE REGISTERS */
                        rc = preset_multiple_registers(&mb_param, SERVER_ID, addr, nb,
                                                        tab_rq_registers);
                        if (rc != nb) {
                                printf("ERROR preset_multiple_registers (%d)\n", rc);
                                printf("Slave = %d, address = %d, nb = %d\n",
                                               SERVER_ID, addr, nb);
                                nb_fail++;
                        } else {
                                rc = read_holding_registers(&mb_param, SERVER_ID, addr, nb,
                                                             tab_rp_registers);
                                if (rc != nb) {
                                        printf("ERROR read_holding_registers (%d)\n", rc);
                                        printf("Slave = %d, address = %d, nb = %d\n",
                                               SERVER_ID, addr, nb);
                                        nb_fail++;
                                } else {
                                        for (i=0; i<nb; i++) {
                                                if (tab_rq_registers[i] != tab_rp_registers[i]) {
                                                        printf("ERROR read_holding_registers\n");
                                                        printf("Slave = %d, address = %d, value %d (0x%X) != %d (0x%X)\n",
                                                               SERVER_ID, addr,
                                                               tab_rq_registers[i], tab_rq_registers[i],
                                                               tab_rp_registers[i], tab_rp_registers[i]);
                                                        nb_fail++;
                                                }
                                        }
                                }
                        }

                }

                printf("Test: ");
                if (nb_fail)
                        printf("%d FAILS\n", nb_fail);
                else
                        printf("SUCCESS\n");
        }

        /* Free the memory */
        free(tab_rq_status);
        free(tab_rp_status);
        free(tab_rq_registers);
        free(tab_rp_registers);

        /* Close the connection */
        modbus_close(&mb_param);

        return 0;
}

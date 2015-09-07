/******************************************************************************
 * Copyright (c) 2004, 2008 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

uint8_t my_inb(uint16_t addr);

uint16_t my_inw(uint16_t addr);

uint32_t my_inl(uint16_t addr);

void my_outb(uint16_t addr, uint8_t val);

void my_outw(uint16_t addr, uint16_t val);

void my_outl(uint16_t addr, uint32_t val);


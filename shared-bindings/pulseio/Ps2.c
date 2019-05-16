/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
 * Copyright (c) 2019 Elvis Pfutzenreuter <epxx@epxx.co>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>

#include "lib/utils/context_manager_helpers.h"
#include "py/objproperty.h"
#include "py/runtime.h"
#include "py/runtime0.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/pulseio/Ps2.h"
#include "shared-bindings/util.h"
#include "supervisor/shared/translate.h"

//| .. currentmodule:: pulseio
//|
//| :class:`Ps2` -- Communicate with a PS/2 keyboard or mouse
//| ========================================================
//|
//| Ps2 implements the PS/2 keyboard/mouse serial protocol, used in
//| legacy devices. It is similar to UART but there are only two 
//| lines (Data and Clock). PS/2 devices are 5V, so bidirectional
//| level converters must be used to connect the I/O lines to pins
//| of  3.3V boards.
//|
//| .. class:: Ps2(data_pin, clock_pin)
//|
//|   Create a Ps2 object associated with the given pins.
//|
//|   :param ~microcontroller.Pin data_pin: Pin tied to data wire.
//|   :param ~microcontroller.Pin clock_pin: Pin tied to clock wire.
//|   This pin must spport interrupts.
//|
//|     import pulseio
//|     import board
//|
//|     kbd = pulseio.Ps2(board.D10, board.D11)
//|
//|     while len(kbd) == 0:
//|         pass
//|     print(kbd.get_byte())
//|     print(kbd.send_byte(0xed))
//|     print(kbd.send_byte(0x02))
//|
STATIC mp_obj_t pulseio_ps2_make_new(const mp_obj_type_t *type, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_datapin, ARG_clkpin };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_datapin, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_clkpin, MP_ARG_REQUIRED | MP_ARG_OBJ },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    assert_pin(args[ARG_clkpin].u_obj, false);
    assert_pin(args[ARG_datapin].u_obj, false);
    const mcu_pin_obj_t* clkpin = MP_OBJ_TO_PTR(args[ARG_clkpin].u_obj);
    assert_pin_free(clkpin);
    const mcu_pin_obj_t* datapin = MP_OBJ_TO_PTR(args[ARG_datapin].u_obj);
    assert_pin_free(datapin);

    pulseio_ps2_obj_t *self = m_new_obj(pulseio_ps2_obj_t);
    self->base.type = &pulseio_ps2_type;

    common_hal_pulseio_ps2_construct(self, datapin, clkpin);

    return MP_OBJ_FROM_PTR(self);
}

//|   .. method:: deinit()
//|
//|      Deinitialises the Ps2 and releases any hardware resources for reuse.
//|
STATIC mp_obj_t pulseio_ps2_deinit(mp_obj_t self_in) {
    pulseio_ps2_obj_t *self = MP_OBJ_TO_PTR(self_in);
    common_hal_pulseio_ps2_deinit(self);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pulseio_ps2_deinit_obj, pulseio_ps2_deinit);

//|   .. method:: __enter__()
//|
//|      No-op used by Context Managers.
//|
//  Provided by context manager helper.

//|   .. method:: __exit__()
//|
//|      Automatically deinitializes the hardware when exiting a context. See
//|      :ref:`lifetime-and-contextmanagers` for more info.
//|
STATIC mp_obj_t pulseio_ps2_obj___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    common_hal_pulseio_ps2_deinit(args[0]);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pulseio_ps2___exit___obj, 4, 4, pulseio_ps2_obj___exit__);

//|   .. method:: get_byte()
//|
//|     Removes and returns the oldest received byte. When buffer
//|     is empty, returns a negative number.
//|
STATIC mp_obj_t pulseio_ps2_obj_get_byte(mp_obj_t self_in) {
    pulseio_ps2_obj_t *self = MP_OBJ_TO_PTR(self_in);
    raise_error_if_deinited(common_hal_pulseio_ps2_deinited(self));

    return MP_OBJ_NEW_SMALL_INT(common_hal_pulseio_ps2_get_byte(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(pulseio_ps2_get_byte_obj, pulseio_ps2_obj_get_byte);

//|   .. method:: send_byte()
//|
//|     Sends a byte to PS/2. Returns the response byte (positive)
//|     or a communication error code (negative). The response is
//|     typically an ACK (0xFA). Some commands return additional data
//|     bytes that should be received via get_byte().
//|
STATIC mp_obj_t pulseio_ps2_obj_send_byte(mp_obj_t self_in, mp_obj_t ob) {
    pulseio_ps2_obj_t *self = MP_OBJ_TO_PTR(self_in);
    raise_error_if_deinited(common_hal_pulseio_ps2_deinited(self));
    mp_int_t b = mp_obj_get_int(ob) & 0xff;
    return MP_OBJ_NEW_SMALL_INT(common_hal_pulseio_ps2_send_byte(self, b));
}
MP_DEFINE_CONST_FUN_OBJ_2(pulseio_ps2_send_byte_obj, pulseio_ps2_obj_send_byte);

//|   .. method:: get_error()
//|
//|     Returns and clears the bitmap of last communication errors.
//|
STATIC mp_obj_t pulseio_ps2_obj_get_error(mp_obj_t self_in) {
    pulseio_ps2_obj_t *self = MP_OBJ_TO_PTR(self_in);
    raise_error_if_deinited(common_hal_pulseio_ps2_deinited(self));

    return MP_OBJ_NEW_SMALL_INT(common_hal_pulseio_ps2_get_error(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(pulseio_ps2_get_error_obj, pulseio_ps2_obj_get_error);

//|   .. method:: __len__()
//|
//|     Returns the number of received bytes in buffer, obtainable
//|     via get_byte()
//|
STATIC mp_obj_t ps2_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    pulseio_ps2_obj_t *self = MP_OBJ_TO_PTR(self_in);
    raise_error_if_deinited(common_hal_pulseio_ps2_deinited(self));
    uint16_t len = common_hal_pulseio_ps2_get_len(self);
    switch (op) {
        case MP_UNARY_OP_BOOL: return mp_obj_new_bool(len != 0);
        case MP_UNARY_OP_LEN: return MP_OBJ_NEW_SMALL_INT(len);
        default: return MP_OBJ_NULL; // op not supported
    }
}

STATIC const mp_rom_map_elem_t pulseio_ps2_locals_dict_table[] = {
    // Methods
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&pulseio_ps2_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&pulseio_ps2___exit___obj) },
    { MP_ROM_QSTR(MP_QSTR_get_byte), MP_ROM_PTR(&pulseio_ps2_get_byte_obj) },
    { MP_ROM_QSTR(MP_QSTR_send_byte), MP_ROM_PTR(&pulseio_ps2_send_byte_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_error), MP_ROM_PTR(&pulseio_ps2_get_error_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pulseio_ps2_locals_dict, pulseio_ps2_locals_dict_table);

const mp_obj_type_t pulseio_ps2_type = {
    { &mp_type_type },
    .name = MP_QSTR_Ps2,
    .make_new = pulseio_ps2_make_new,
    .unary_op = ps2_unary_op,
    .locals_dict = (mp_obj_dict_t*)&pulseio_ps2_locals_dict,
};

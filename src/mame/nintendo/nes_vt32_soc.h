// license:BSD-3-Clause
// copyright-holders:David Haywood
#ifndef MAME_NINTENDO_NES_VT32_SOC_H
#define MAME_NINTENDO_NES_VT32_SOC_H

#pragma once

#include "m6502_swap_op_d5_d6.h"
#include "nes_vt09_soc.h"
#include "rp2a03_vtscr.h"

#include "cpu/m6502/rp2a03.h"
#include "sound/nes_apu_vt.h"
#include "video/ppu2c0x_vt.h"

#include "screen.h"
#include "speaker.h"


class nes_vt32_soc_device : public nes_vt09_soc_device
{
public:
	nes_vt32_soc_device(const machine_config& mconfig, const char* tag, device_t* owner, u32 clock);

protected:
	nes_vt32_soc_device(const machine_config& mconfig, device_type type, const char* tag, device_t* owner, u32 clock);

	virtual void device_add_mconfig(machine_config& config) override ATTR_COLD;

	void nes_vt32_soc_map(address_map &map) ATTR_COLD;

	u8 vtfp_4119_r();
	void vtfp_411e_encryption_state_w(u8 data);
	void vtfp_412c_extbank_w(u8 data);
	u8 vtfp_412d_r();
	void vtfp_4242_w(u8 data);
	void vtfp_4a00_w(u8 data);
	void vtfp_411d_w(u8 data);
	u8 vthh_414a_r();
};

class nes_vt32_soc_pal_device : public nes_vt32_soc_device
{
public:
	nes_vt32_soc_pal_device(const machine_config& mconfig, const char* tag, device_t* owner, u32 clock);

protected:
	virtual void device_add_mconfig(machine_config& config) override ATTR_COLD;
};


DECLARE_DEVICE_TYPE(NES_VT32_SOC, nes_vt32_soc_device)
DECLARE_DEVICE_TYPE(NES_VT32_SOC_PAL, nes_vt32_soc_pal_device)

#endif // MAME_NINTENDO_NES_VT32_SOC_H

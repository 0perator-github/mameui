// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  drawd3d.cpp - Win32 Direct3D implementation
//
//============================================================

// MAME headers
#include "emu.h"
#include "emuopts.h"
#include "render.h"
#include "rendutil.h"
#include "screen.h"

// MAMEOS headers
#include "drawd3d.h"

#include "d3d/d3dhlsl.h"

#include "render_module.h"

// from OSD implementation
#include "window.h"
#include "winmain.h"

// general OSD headers
#include "modules/monitor/monitor_module.h"

// lib/util
#include "aviio.h"

#include <utility>


//============================================================
//  OSD MODULE
//============================================================

namespace osd {

namespace {

class video_d3d : public osd_module, public render_module
{
public:
	video_d3d()
		: osd_module(OSD_RENDERER_PROVIDER, "d3d")
		, m_options(nullptr)
	{
	}

	virtual bool probe() override;
	virtual int init(osd_interface &osd, osd_options const &options) override;
	virtual void exit() override;

	virtual std::unique_ptr<osd_renderer> create(osd_window &window) override;

protected:
	virtual unsigned flags() const override { return FLAG_INTERACTIVE; }

private:
	using d3d9_create_fn = IDirect3D9 *(WINAPI *)(UINT);

	dynamic_module::ptr m_d3d9_dll;
	Microsoft::WRL::ComPtr<IDirect3D9> m_d3dobj;
	osd_options const *m_options;
};


//============================================================
//  video_d3d::probe
//============================================================

bool video_d3d::probe()
{
	// do a dry run of loading the Direct3D 9 DLL
	return dynamic_module::open({ "d3d9.dll" })->bind<d3d9_create_fn>("Direct3DCreate9") != nullptr;
}


//============================================================
//  video_d3d::init
//============================================================

int video_d3d::init(osd_interface &osd, osd_options const &options)
{
	m_options = &options;

	m_d3d9_dll = dynamic_module::open({ "d3d9.dll" });
	auto const d3d9_create_ptr = m_d3d9_dll->bind<d3d9_create_fn>("Direct3DCreate9");
	if (!d3d9_create_ptr)
	{
		osd_printf_warning("Direct3D: Could not find Direct3DCreate9 function in d3d9.dll\n");
		m_d3d9_dll.reset();
		m_options = nullptr;
		return -1;
	}

	m_d3dobj.Attach((d3d9_create_ptr)(D3D_SDK_VERSION));
	if (!m_d3dobj)
	{
		osd_printf_warning("Direct3D: Unable to initialize Direct3D 9\n");
		m_d3d9_dll.reset();
		m_options = nullptr;
		return -1;
	}

	osd_printf_verbose("Direct3D: Using Direct3D 9\n");

	return 0;
}


//============================================================
//  video_d3d::exit
//============================================================

void video_d3d::exit()
{
	m_d3dobj.Reset();
	m_d3d9_dll.reset();
	m_options = nullptr;
}


//============================================================
//  video_d3d::create
//============================================================

std::unique_ptr<osd_renderer> video_d3d::create(osd_window &window)
{
	return std::make_unique<renderer_d3d9>(window, m_d3dobj);
}

} // anonymous namespace

} // namespace osd

MODULE_DEFINITION(RENDERER_D3D, osd::video_d3d)



//============================================================
//  CONSTANTS
//============================================================

enum
{
	TEXTURE_TYPE_PLAIN,
	TEXTURE_TYPE_DYNAMIC,
	TEXTURE_TYPE_SURFACE
};


//============================================================
//  INLINES
//============================================================

static inline BOOL GetClientRectExceptMenu(HWND hWnd, PRECT pRect, BOOL fullscreen)
{
	static HMENU last_menu;
	static RECT last_rect;
	static RECT cached_rect;
	HMENU menu = GetMenu(hWnd);
	BOOL result = GetClientRect(hWnd, pRect);

	if (!fullscreen || !menu)
		return result;

	// to avoid flicker use cache if we can use
	if (last_menu != menu || memcmp(&last_rect, pRect, sizeof *pRect) != 0)
	{
		last_menu = menu;
		last_rect = *pRect;

		SetMenu(hWnd, nullptr);
		result = GetClientRect(hWnd, &cached_rect);
		SetMenu(hWnd, menu);
	}

	*pRect = cached_rect;
	return result;
}


static inline uint32_t ycc_to_rgb(uint8_t y, uint8_t cb, uint8_t cr)
{
	/* original equations:

	    C = Y - 16
	    D = Cb - 128
	    E = Cr - 128

	    R = clip(( 298 * C           + 409 * E + 128) >> 8)
	    G = clip(( 298 * C - 100 * D - 208 * E + 128) >> 8)
	    B = clip(( 298 * C + 516 * D           + 128) >> 8)

	    R = clip(( 298 * (Y - 16)                    + 409 * (Cr - 128) + 128) >> 8)
	    G = clip(( 298 * (Y - 16) - 100 * (Cb - 128) - 208 * (Cr - 128) + 128) >> 8)
	    B = clip(( 298 * (Y - 16) + 516 * (Cb - 128)                    + 128) >> 8)

	    R = clip(( 298 * Y - 298 * 16                        + 409 * Cr - 409 * 128 + 128) >> 8)
	    G = clip(( 298 * Y - 298 * 16 - 100 * Cb + 100 * 128 - 208 * Cr + 208 * 128 + 128) >> 8)
	    B = clip(( 298 * Y - 298 * 16 + 516 * Cb - 516 * 128                        + 128) >> 8)

	    R = clip(( 298 * Y - 298 * 16                        + 409 * Cr - 409 * 128 + 128) >> 8)
	    G = clip(( 298 * Y - 298 * 16 - 100 * Cb + 100 * 128 - 208 * Cr + 208 * 128 + 128) >> 8)
	    B = clip(( 298 * Y - 298 * 16 + 516 * Cb - 516 * 128                        + 128) >> 8)
	*/
	int r, g, b, common;

	common = 298 * y - 298 * 16;
	r = (common +                        409 * cr - 409 * 128 + 128) >> 8;
	g = (common - 100 * cb + 100 * 128 - 208 * cr + 208 * 128 + 128) >> 8;
	b = (common + 516 * cb - 516 * 128                        + 128) >> 8;

	if (r < 0) r = 0;
	else if (r > 255) r = 255;
	if (g < 0) g = 0;
	else if (g > 255) g = 255;
	if (b < 0) b = 0;
	else if (b > 255) b = 255;

	return rgb_t(0xff, r, g, b);
}


//============================================================
//  drawd3d_window_init
//============================================================

int renderer_d3d9::create()
{
	if (!initialize())
	{
		osd_printf_error("Unable to initialize Direct3D 9\n");
		return 1;
	}

	return 0;
}

void renderer_d3d9::toggle_fsfx()
{
	set_toggle(true);
}

void renderer_d3d9::record()
{
	if (m_shaders != nullptr)
	{
		m_shaders->record_movie();
	}
}

void renderer_d3d9::add_audio_to_recording(const int16_t *buffer, int samples_this_frame)
{
	if (m_shaders != nullptr)
	{
		m_shaders->record_audio(buffer, samples_this_frame);
	}
}

void renderer_d3d9::save()
{
	if (m_shaders != nullptr)
	{
		m_shaders->save_snapshot();
	}
}


//============================================================
//  drawd3d_window_get_primitives
//============================================================

render_primitive_list *renderer_d3d9::get_primitives()
{
	RECT client;
	HWND hWnd = dynamic_cast<win_window_info &>(window()).platform_window();
	if (IsIconic(hWnd))
		return nullptr;

	GetClientRectExceptMenu(hWnd, &client, window().fullscreen());
	if (rect_width(&client) > 0 && rect_height(&client) > 0)
	{
		window().target()->set_bounds(rect_width(&client), rect_height(&client), window().pixel_aspect());
		window().target()->set_max_update_rate((get_refresh() == 0) ? get_origmode().RefreshRate : get_refresh());
	}
	if (m_shaders != nullptr)
	{
		// do not transform primitives (scale, offset) if shaders are enabled, the shaders will handle the transformation
		window().target()->set_transform_container(!m_shaders->enabled());
	}
	return &window().target()->get_primitives();
}


//============================================================
//  drawd3d_window_draw
//============================================================

int renderer_d3d9::draw(const int update)
{
	int check = pre_window_draw_check();
	if (check >= 0)
		return check;

	begin_frame();

	// reset blend mode
	set_blendmode(BLENDMODE_NONE);

	process_primitives();

	end_frame();

	return 0;
}

void renderer_d3d9::set_texture(texture_info *texture)
{
	if (texture != m_last_texture)
	{
		m_last_texture = texture;
		m_last_texture_flags = (texture == nullptr ? 0 : texture->get_flags());
		if (m_shaders->enabled())
		{
			m_shaders->set_texture(texture);
		}

		HRESULT result = m_device->SetTexture(0, (texture == nullptr) ? get_default_texture()->get_finaltex() : texture->get_finaltex());
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device set_texture call\n", result);
	}
}

void renderer_d3d9::set_filter(int filter)
{
	if (filter != m_last_filter)
	{
		m_last_filter = filter;
		HRESULT result = m_device->SetSamplerState(0, D3DSAMP_MINFILTER, filter ? D3DTEXF_LINEAR : D3DTEXF_POINT);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetSamplerState call\n", result);
		result = m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, filter ? D3DTEXF_LINEAR : D3DTEXF_POINT);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetSamplerState call\n", result);
		result = m_device->SetSamplerState(1, D3DSAMP_MINFILTER, filter ? D3DTEXF_LINEAR : D3DTEXF_POINT);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetSamplerState call\n", result);
		result = m_device->SetSamplerState(1, D3DSAMP_MAGFILTER, filter ? D3DTEXF_LINEAR : D3DTEXF_POINT);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetSamplerState call\n", result);
	}
}

void renderer_d3d9::set_wrap(unsigned int wrap)
{
	if (wrap != m_last_wrap)
	{
		m_last_wrap = wrap;
		HRESULT result = m_device->SetSamplerState(0, D3DSAMP_ADDRESSU, wrap);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetSamplerState call\n", result);
		result = m_device->SetSamplerState(0, D3DSAMP_ADDRESSV, wrap);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetSamplerState call\n", result);
		result = m_device->SetSamplerState(1, D3DSAMP_ADDRESSU, wrap);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetSamplerState call\n", result);
		result = m_device->SetSamplerState(1, D3DSAMP_ADDRESSV, wrap);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetSamplerState call\n", result);
	}
}

void renderer_d3d9::set_modmode(int modmode)
{
	if (modmode != m_last_modmode)
	{
		m_last_modmode = modmode;
		HRESULT result = m_device->SetTextureStageState(0, D3DTSS_COLOROP, modmode);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetTextureStageState call\n", result);
		result = m_device->SetTextureStageState(1, D3DTSS_COLOROP, modmode);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetTextureStageState call\n", result);
	}
}

void renderer_d3d9::set_blendmode(int blendmode)
{
	int blendenable;
	int blendop;
	int blendsrc;
	int blenddst;

	// choose the parameters
	switch (blendmode)
	{
		default:
		case BLENDMODE_NONE:
			blendenable = FALSE;
			blendop = D3DBLENDOP_ADD;
			blendsrc = D3DBLEND_SRCALPHA;
			blenddst = D3DBLEND_INVSRCALPHA;
			break;
		case BLENDMODE_ALPHA:
			blendenable = TRUE;
			blendop = D3DBLENDOP_ADD;
			blendsrc = D3DBLEND_SRCALPHA;
			blenddst = D3DBLEND_INVSRCALPHA;
			break;
		case BLENDMODE_RGB_MULTIPLY:
			blendenable = TRUE;
			blendop = D3DBLENDOP_ADD;
			blendsrc = D3DBLEND_DESTCOLOR;
			blenddst = D3DBLEND_ZERO;
			break;
		case BLENDMODE_ADD:
			blendenable = TRUE;
			blendop = D3DBLENDOP_ADD;
			blendsrc = D3DBLEND_SRCALPHA;
			blenddst = D3DBLEND_ONE;
			break;
	}

	// adjust the bits that changed
	bool new_blend_enable = false;
	if (blendenable != m_last_blendenable)
	{
		m_last_blendenable = blendenable;
		if (blendenable)
		{
			new_blend_enable  = true;
		}
		HRESULT result = m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, blendenable);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetRenderState call\n", result);
	}

	if (blendop != m_last_blendop || new_blend_enable)
	{
		m_last_blendop = blendop;
		HRESULT result = m_device->SetRenderState(D3DRS_BLENDOP, blendop);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetRenderState call\n", result);
	}

	if (blendsrc != m_last_blendsrc || new_blend_enable)
	{
		m_last_blendsrc = blendsrc;
		HRESULT result = m_device->SetRenderState(D3DRS_SRCBLEND, blendsrc);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetRenderState call\n", result);
	}

	if (blenddst != m_last_blenddst || new_blend_enable)
	{
		m_last_blenddst = blenddst;
		HRESULT result = m_device->SetRenderState(D3DRS_DESTBLEND, blenddst);
		if (FAILED(result))
			osd_printf_verbose("Direct3D: Error %08lX during device SetRenderState call\n", result);
	}
}

void renderer_d3d9::reset_render_states()
{
	// this ensures subsequent calls to the above setters will force-update the data
	m_last_texture = (texture_info *)~0;
	m_last_filter = -1;
	m_last_blendenable = -1;
	m_last_blendop = -1;
	m_last_blendsrc = -1;
	m_last_blenddst = -1;
	m_last_wrap = (D3DTEXTUREADDRESS)-1;
}

d3d_texture_manager::d3d_texture_manager(renderer_d3d9 &d3d, IDirect3D9 *d3dobj)
	: m_renderer(d3d)
	, m_default_texture(nullptr)
{
	D3DCAPS9 caps;
	HRESULT result = d3dobj->GetDeviceCaps(d3d.get_adapter(), D3DDEVTYPE_HAL, &caps);
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during GetDeviceCaps call\n", result);

	// get texture caps
	m_texture_caps = caps.TextureCaps;
	m_texture_max_aspect = caps.MaxTextureAspectRatio;
	m_texture_max_width = caps.MaxTextureWidth;
	m_texture_max_height = caps.MaxTextureHeight;

	// pick a YUV texture format
	m_yuv_format = D3DFMT_UYVY;
	result = d3dobj->CheckDeviceFormat(d3d.get_adapter(), D3DDEVTYPE_HAL, d3d.get_pixel_format(), 0, D3DRTYPE_TEXTURE, D3DFMT_UYVY);
	if (FAILED(result))
	{
		m_yuv_format = D3DFMT_YUY2;
		result = d3dobj->CheckDeviceFormat(d3d.get_adapter(), D3DDEVTYPE_HAL, d3d.get_pixel_format(), 0, D3DRTYPE_TEXTURE, D3DFMT_YUY2);
		if (FAILED(result))
			m_yuv_format = D3DFMT_A8R8G8B8;
	}
	osd_printf_verbose("Direct3D: YUV format = %s\n", (m_yuv_format == D3DFMT_YUY2) ? "YUY2" : (m_yuv_format == D3DFMT_UYVY) ? "UYVY" : "RGB");

	// set the max texture size
	d3d.window().target()->set_max_texture_size(m_texture_max_width, m_texture_max_height);
	osd_printf_verbose("Direct3D: Max texture size = %dx%d\n", m_texture_max_width, m_texture_max_height);
}

void d3d_texture_manager::create_resources()
{
	m_default_bitmap.allocate(8, 8);
	if (m_default_bitmap.valid())
	{
		m_default_bitmap.fill(rgb_t(0xff,0xff,0xff,0xff));

		// fake in the basic data so it looks like it came from render.c
		render_texinfo texture;
		texture.base = m_default_bitmap.raw_pixptr(0);
		texture.rowpixels = m_default_bitmap.rowpixels();
		texture.width = m_default_bitmap.width();
		texture.height = m_default_bitmap.height();
		texture.palette = nullptr;
		texture.seqid = 0;
		texture.unique_id = ~0ULL;
		texture.old_id = ~0ULL;

		// now create it
		auto tex = std::make_unique<texture_info>(*this, &texture, m_renderer.window().prescale(), PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXFORMAT(TEXFORMAT_ARGB32));
		m_default_texture = tex.get();
		m_texture_list.push_back(std::move(tex));
	}
}

void d3d_texture_manager::delete_resources()
{
	// is part of m_texlist and will be free'd there
	m_default_texture = nullptr;

	// free all textures
	m_texture_list.clear();
}

uint32_t d3d_texture_manager::texture_compute_hash(const render_texinfo *texture, uint32_t flags)
{
	return (uintptr_t)texture->base ^ (flags & (PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK));
}

texture_info *d3d_texture_manager::find_texinfo(const render_texinfo *texinfo, uint32_t flags)
{
	const uint32_t hash = texture_compute_hash(texinfo, flags);

	// find a match
	for (auto it = m_texture_list.begin(); it != m_texture_list.end(); it++)
	{
		const uint32_t test_screen = uint32_t((*it)->get_texinfo().unique_id >> 57);
		const uint32_t test_page = uint32_t((*it)->get_texinfo().unique_id >> 56) & 1;
		const uint32_t prim_screen = uint32_t(texinfo->unique_id >> 57);
		const uint32_t prim_page = uint32_t(texinfo->unique_id >> 56) & 1;
		if ((test_screen != prim_screen) || (test_page != prim_page))
			continue;

		if ((*it)->get_hash() == hash &&
			(*it)->get_texinfo().base == texinfo->base &&
			(*it)->get_texinfo().width == texinfo->width &&
			(*it)->get_texinfo().height == texinfo->height &&
			(((*it)->get_flags() ^ flags) & (PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK)) == 0)
		{
			return (*it).get();
		}
	}

	return nullptr;
}

renderer_d3d9::renderer_d3d9(osd_window &window, const IDirect3D9Ptr &d3dobj)
	: osd_renderer(window)
	, m_d3dobj(d3dobj)
	, m_adapter(0)
	, m_width(0)
	, m_height(0)
	, m_refresh(0)
	, m_create_error_count(0)
	, m_post_fx_available(true)
	, m_gamma_supported(0)
	, m_pixformat()
	, m_lockedbuf(nullptr)
	, m_numverts(0)
	, m_vectorbatch(nullptr)
	, m_batchindex(0)
	, m_numpolys(0)
	, m_toggle(false)
	, m_screen_format()
	, m_last_texture(nullptr)
	, m_last_texture_flags(0)
	, m_last_blendenable(0)
	, m_last_blendop(0)
	, m_last_blendsrc(0)
	, m_last_blenddst(0)
	, m_last_filter(0)
	, m_last_wrap()
	, m_last_modmode(0)
	, m_shaders(nullptr)
	, m_texture_manager()
{
}

int renderer_d3d9::initialize()
{
	osd_printf_verbose("Direct3D: Initialize\n");

	// configure the adapter for the mode we want
	if (config_adapter_mode())
	{
		return false;
	}

	// create the device immediately for the full screen case (defer for window mode in update_window_size())
	if (window().fullscreen() && device_create(dynamic_cast<win_window_info &>(window()).main_window()->platform_window()))
	{
		return false;
	}

	return true;
}

int renderer_d3d9::pre_window_draw_check()
{
	// if we're in the middle of resizing, leave things alone
	if (dynamic_cast<win_window_info &>(window()).m_resize_state == win_window_info::RESIZE_STATE_RESIZING)
		return 0;

	// check if shaders should be toggled
	if (m_toggle)
	{
		m_toggle = false;

		// free resources
		device_delete_resources();

		m_shaders->toggle();
		m_sliders_dirty = true;

		// re-create resources
		if (device_create_resources())
		{
			osd_printf_verbose("Direct3D: failed to recreate resources for device; failing permanently\n");
			device_delete();
			return 1;
		}
	}

	// if we have a device, check the cooperative level
	if (m_device)
	{
		if (device_test_cooperative())
		{
			return 1;
		}
	}

	// in window mode, we need to track the window size
	if (!window().fullscreen() || !m_device)
	{
		// if the size changes, skip this update since the render target will be out of date
		if (update_window_size())
			return 0;

		// if we have no device, after updating the size, return an error so GDI can try
		if (!m_device)
			return 1;
	}

	return -1;
}

void d3d_texture_manager::update_textures()
{
	for (render_primitive &prim : *m_renderer.window().m_primlist)
	{
		if (prim.texture.base != nullptr)
		{
			texture_info *texture = find_texinfo(&prim.texture, prim.flags);
			if (texture == nullptr)
			{
				const int prescale = m_renderer.get_shaders()->enabled() ? 1 : m_renderer.window().prescale();

				auto tex = std::make_unique<texture_info>(*this, &prim.texture, prescale, prim.flags);
				texture = tex.get();
				m_texture_list.push_back(std::move(tex));
			}
			else
			{
				// if there is one, but with a different seqid, copy the data
				if (texture->get_texinfo().seqid != prim.texture.seqid)
				{
					texture->set_data(&prim.texture, prim.flags);
					texture->get_texinfo().seqid = prim.texture.seqid;
				}
			}
		}
	}

	if (!m_renderer.get_shaders()->enabled())
		return;

	int screen_index = 0;
	for (render_primitive &prim : *m_renderer.window().m_primlist)
	{
		if (PRIMFLAG_GET_SCREENTEX(prim.flags))
		{
			if (!m_renderer.get_shaders()->get_texture_target(&prim, prim.texture.width, prim.texture.height, screen_index))
			{
				if (!m_renderer.get_shaders()->create_texture_target(&prim, prim.texture.width, prim.texture.height, screen_index))
				{
					m_renderer.set_post_fx_unavailable();
					break;
				}
			}
			screen_index++;
		}
		else if (PRIMFLAG_GET_VECTORBUF(prim.flags))
		{
			if (!m_renderer.get_shaders()->get_vector_target(&prim, screen_index))
			{
				if (!m_renderer.get_shaders()->create_vector_target(&prim, screen_index))
				{
					m_renderer.set_post_fx_unavailable();
					break;
				}
			}
			screen_index++;
		}
	}
}

void renderer_d3d9::begin_frame()
{
	HRESULT result = m_device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xff,0,0,0), 0, 0);
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during device clear call\n", result);

	window().m_primlist->acquire_lock();

	// first update any textures
	m_texture_manager->update_textures();

	// begin the scene
	result = m_device->BeginScene();
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during device BeginScene call\n", result);

	if (m_shaders->enabled())
		m_shaders->begin_frame(window().m_primlist);

	// set an initial default texture
	set_texture(nullptr);
}

void renderer_d3d9::process_primitives()
{
	// loop over line primitives
	int vector_count = 0;
	for (render_primitive &prim : *window().m_primlist)
	{
		if (prim.type == render_primitive::LINE && PRIMFLAG_GET_VECTOR(prim.flags))
		{
			vector_count++;
		}
	}

	// Rotating index for vector time offsets
	for (render_primitive &prim : *window().m_primlist)
	{
		switch (prim.type)
		{
			case render_primitive::LINE:
				if (PRIMFLAG_GET_VECTOR(prim.flags))
				{
					if (vector_count > 0)
					{
						batch_vectors(vector_count);
						vector_count = 0;
					}
				}
				else
				{
					draw_line(prim);
				}
				break;

			case render_primitive::QUAD:
				draw_quad(prim);
				break;

			default:
				throw emu_fatalerror("Unexpected render_primitive type");
		}
	}
}

void renderer_d3d9::end_frame()
{
	window().m_primlist->release_lock();

	// flush any pending polygons
	primitive_flush_pending();

	if (m_shaders->enabled())
		m_shaders->end_frame();

	// finish the scene
	HRESULT result = m_device->EndScene();
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during device end_scene call\n", result);

	// present the current buffers
	result = m_device->Present(nullptr, nullptr, nullptr, nullptr);
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during device present call\n", result);
}

void renderer_d3d9::update_presentation_parameters()
{
	memset(&m_presentation, 0, sizeof(m_presentation));
	m_presentation.BackBufferWidth = m_width;
	m_presentation.BackBufferHeight = m_height;
	m_presentation.BackBufferFormat = m_pixformat;
	m_presentation.BackBufferCount = video_config.triplebuf ? 2 : 1;
	m_presentation.MultiSampleType = D3DMULTISAMPLE_NONE;
	m_presentation.SwapEffect = D3DSWAPEFFECT_DISCARD;
	m_presentation.hDeviceWindow = dynamic_cast<win_window_info &>(window()).platform_window();
	m_presentation.Windowed = !window().fullscreen();
	m_presentation.EnableAutoDepthStencil = FALSE;
	m_presentation.AutoDepthStencilFormat = D3DFMT_D16;
	m_presentation.Flags = 0;
	m_presentation.FullScreen_RefreshRateInHz = m_refresh;
	m_presentation.PresentationInterval = (
		(video_config.triplebuf && window().fullscreen())
		|| video_config.waitvsync
		|| video_config.syncrefresh)
			? D3DPRESENT_INTERVAL_ONE
			: D3DPRESENT_INTERVAL_IMMEDIATE;
}


void renderer_d3d9::update_gamma_ramp()
{
	if (!m_gamma_supported)
	{
		return;
	}

	// set the gamma if we need to
	if (window().fullscreen())
	{
		// only set the gamma if it's not 1.0
		auto &options = downcast<windows_options &>(window().machine().options());
		float brightness = options.full_screen_brightness();
		float contrast = options.full_screen_contrast();
		float gamma = options.full_screen_gamma();
		if (brightness != 1.0f || contrast != 1.0f || gamma != 1.0f)
		{
			D3DGAMMARAMP ramp;

			for (int i = 0; i < 256; i++)
			{
				ramp.red[i] = ramp.green[i] = ramp.blue[i] = apply_brightness_contrast_gamma(i, brightness, contrast, gamma) << 8;
			}

			m_device->SetGammaRamp(0, 0, &ramp);
		}
	}
}


//============================================================
//  device_create
//============================================================

int renderer_d3d9::device_create(HWND hwnd)
{
	// identify the actual window; this is needed so that -attach_window
	// can work on a non-root HWND
	HWND device_hwnd = GetAncestor(hwnd, GA_ROOT);

	// if a device exists, free it
	if (m_device)
		device_delete();

	// verify the caps
	if (!device_verify_caps())
		return 1;

	m_texture_manager = std::make_unique<d3d_texture_manager>(*this, m_d3dobj.Get());

	// try for XRGB first
	m_screen_format = D3DFMT_X8R8G8B8;
	HRESULT result = m_d3dobj->CheckDeviceFormat(m_adapter, D3DDEVTYPE_HAL, m_pixformat, D3DUSAGE_DYNAMIC, D3DRTYPE_TEXTURE, m_screen_format);
	if (FAILED(result))
	{
		// if not, try for ARGB
		m_screen_format = D3DFMT_A8R8G8B8;
		result = m_d3dobj->CheckDeviceFormat(m_adapter, D3DDEVTYPE_HAL, m_pixformat, D3DUSAGE_DYNAMIC, D3DRTYPE_TEXTURE, m_screen_format);
		if (FAILED(result))
		{
			osd_printf_error("Error: unable to configure a screen texture format\n");
			return 1;
		}
	}

	// initialize the D3D presentation parameters
	update_presentation_parameters();

	// create the D3D device
	result = m_d3dobj->CreateDevice(
			m_adapter,
			D3DDEVTYPE_HAL,
			device_hwnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
			&m_presentation,
			&m_device);
	if (FAILED(result))
	{
		// if we got a "DEVICELOST" error, it may be transitory; count it and only fail if
		// we exceed a threshold
		if (result == D3DERR_DEVICELOST)
		{
			m_create_error_count++;
			if (m_create_error_count < 10)
			{
				return 0;
			}
		}

		//  fatal error if we just can't do it
		osd_printf_error("Unable to create the Direct3D device (%08X)\n", (uint32_t)result);
		return 1;
	}
	m_create_error_count = 0;
	osd_printf_verbose("Direct3D: Device created at %dx%d\n", m_width, m_height);

	update_gamma_ramp();

	return device_create_resources();
}


//============================================================
//  device_create_resources
//============================================================

int renderer_d3d9::device_create_resources()
{
	// create shaders only once
	if (!m_shaders)
		m_shaders = std::make_unique<shaders>();

	if (m_shaders->init(m_d3dobj.Get(), &window().machine(), this))
	{
		m_post_fx_available = true;
		m_shaders->init_slider_list();
		m_sliders_dirty = true;
	}

	// create resources
	if (m_shaders->create_resources())
	{
		osd_printf_verbose("Direct3D: failed to create HLSL resources for device\n");
		return 1;
	}

	// allocate a vertex buffer to use
	HRESULT result = m_device->CreateVertexBuffer(
			sizeof(vertex) * VERTEX_BUFFER_SIZE,
			D3DUSAGE_DYNAMIC | D3DUSAGE_SOFTWAREPROCESSING | D3DUSAGE_WRITEONLY,
			VERTEX_BASE_FORMAT | (m_shaders->enabled() ? D3DFVF_XYZW : D3DFVF_XYZRHW),
			D3DPOOL_DEFAULT,
			&m_vertexbuf,
			nullptr);
	if (FAILED(result))
	{
		osd_printf_error("Error creating vertex buffer (%08X)\n", (uint32_t)result);
		return 1;
	}

	// set the vertex format
	result = m_device->SetFVF(
			D3DFORMAT(VERTEX_BASE_FORMAT | (m_shaders->enabled() ? D3DFVF_XYZW : D3DFVF_XYZRHW)));
	if (FAILED(result))
	{
		osd_printf_error("Error setting vertex format (%08X)\n", (uint32_t)result);
		return 1;
	}

	// set the fixed render state
	result = m_device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	result = m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	result = m_device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
	result = m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	result = m_device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	result = m_device->SetRenderState(D3DRS_LASTPIXEL, TRUE);
	result = m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	result = m_device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
	result = m_device->SetRenderState(D3DRS_ALPHAREF, 0);
	result = m_device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	result = m_device->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	result = m_device->SetRenderState(D3DRS_FOGENABLE, FALSE);
	result = m_device->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
	result = m_device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	result = m_device->SetRenderState(D3DRS_WRAP0, FALSE);
	result = m_device->SetRenderState(D3DRS_CLIPPING, TRUE);
	result = m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
	result = m_device->SetRenderState(D3DRS_COLORVERTEX, TRUE);

	result = m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	result = m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	result = m_device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
	result = m_device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

	// reset the local states to force updates
	reset_render_states();

	// clear the buffer
	result = m_device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0,0,0,0), 0, 0);
	result = m_device->Present(nullptr, nullptr, nullptr, nullptr);

	m_texture_manager->create_resources();

	return 0;
}


//============================================================
//  device_delete
//============================================================

renderer_d3d9::~renderer_d3d9()
{
	device_delete();
}

void renderer_d3d9::device_delete()
{
	// free our base resources
	device_delete_resources();

	// we do not delete the HLSL interface here

	m_texture_manager.reset();

	// free the device itself
	m_device.Reset();
}


//============================================================
//  device_delete_resources
//============================================================

void renderer_d3d9::device_delete_resources()
{
	if (m_shaders)
		m_shaders->delete_resources();

	if (m_texture_manager)
		m_texture_manager->delete_resources();

	// free the vertex buffer
	m_vertexbuf.Reset();
}


//============================================================
//  device_verify_caps
//============================================================

bool renderer_d3d9::device_verify_caps()
{
	bool success = true;

	D3DCAPS9 caps;
	HRESULT result = m_d3dobj->GetDeviceCaps(m_adapter, D3DDEVTYPE_HAL, &caps);
	if (FAILED(result))
	{
		osd_printf_verbose("Direct3D: Error %08lX during GetDeviceCaps call\n", result);
		return false;
	}

	if (caps.MaxPixelShader30InstructionSlots < 512)
	{
		osd_printf_verbose("Direct3D: Warning - Device does not support Pixel Shader 3.0, falling back to non-PS rendering\n");
		set_post_fx_unavailable();
	}

	// verify presentation capabilities
	if (!(caps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE))
	{
		osd_printf_verbose("Direct3D Error: Your graphics card does not support immediate presentation.\n");
		success = false;
	}
	if (!(caps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE))
	{
		osd_printf_verbose("Direct3D Error: Your graphics card does not support per-refresh presentation.\n");
		success = false;
	}

	// verify device capabilities
	if (!(caps.DevCaps & D3DDEVCAPS_CANRENDERAFTERFLIP))
	{
#if defined(MAMEUI_WINAPP) // MAMEUI: 0perator, "Dev before me switched these error messages to verbose."
        osd_printf_verbose("Direct3D Error: Your graphics card does not support rendering after a page\n");
        osd_printf_verbose("flip.\n");
#else
		osd_printf_error("Direct3D Error: Your graphics card does not support rendering after a page\n");
		osd_printf_error("flip.\n");
#endif
		success = false;
	}

	if (!(caps.DevCaps & D3DDEVCAPS_HWRASTERIZATION))
	{
#if defined(MAMEUI_WINAPP) // MAMEUI: More error messages switched to verbose.
        osd_printf_verbose("Direct3D Error: Your graphics card does not support hardware rendering.\n");
#else
		osd_printf_error("Direct3D Error: Your graphics card does not support hardware rendering.\n");
#endif
		success = false;
	}

	// verify texture operation capabilities
	if (!(caps.TextureOpCaps & D3DTEXOPCAPS_MODULATE))
	{
#if defined(MAMEUI_WINAPP) // MAMEUI: More error messages switched to verbose.
        osd_printf_verbose("Direct3D Error: Your graphics card does not support modulate-type blending.\n");
#else
		osd_printf_error("Direct3D Error: Your graphics card does not support modulate-type blending.\n");
#endif
		success = false;
	}

	if (caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)
	{
#if defined(MAMEUI_WINAPP) // MAMEUI: More error messages switched to verbose.
        osd_printf_verbose("Direct3D Error: Your graphics card does not fully support non-power-of-two\n");
        osd_printf_verbose("textures.\n");
#else
		osd_printf_error("Direct3D Error: Your graphics card does not fully support non-power-of-two\n");
		osd_printf_error("textures.\n");
#endif
		success = false;
	}

	if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
	{
#if defined(MAMEUI_WINAPP) // MAMEUI: More error messages switched to verbose.
        osd_printf_verbose("Direct3D Error: Your graphics card does not support non-power-of-two textures.\n");
#else
		osd_printf_error("Direct3D Error: Your graphics card does not support non-power-of-two textures.\n");
#endif
		success = false;
	}
	if (caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
	{
#if defined(MAMEUI_WINAPP) // MAMEUI: More error messages switched to verbose.
        osd_printf_verbose("Direct3D Error: Your graphics card does not support non-square textures.\n");
#else
		osd_printf_error("Direct3D Error: Your graphics card does not support non-square textures.\n");
#endif
		success = false;
	}

	// verify texture formats
	result = m_d3dobj->CheckDeviceFormat(m_adapter, D3DDEVTYPE_HAL, m_pixformat, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8);
	if (FAILED(result))
	{
#if defined(MAMEUI_WINAPP) // MAMEUI: More error messages switched to verbose.
        osd_printf_verbose("Direct3D Error: Your graphics card does not support the A8R8G8B8 texture format.\n");
#else
		osd_printf_error("Direct3D Error: Your graphics card does not support the A8R8G8B8 texture format.\n");
#endif
		success = false;
	}

	if (!success)
	{
#if defined(MAMEUI_WINAPP) // MAMEUI: More error messages switched to verbose.
        osd_printf_verbose("This feature or features are required to use the Direct3D renderer. Please\n");
        osd_printf_verbose("select another renderer using the -video option or contact the MAME developers\n");
        osd_printf_verbose("with information about your system.\n");
#else
		osd_printf_error("This feature or features are required to use the Direct3D renderer. Please\n");
		osd_printf_error("select another renderer using the -video option or contact the MAME developers\n");
		osd_printf_error("with information about your system.\n");
		return false;
#endif
	}

	m_gamma_supported = ((caps.Caps2 & D3DCAPS2_FULLSCREENGAMMA) != 0);
	if (!m_gamma_supported)
	{
		osd_printf_warning("Direct3D: Warning - device does not support full screen gamma correction.\n");
	}

	return true;
}


//============================================================
//  device_test_cooperative
//============================================================

int renderer_d3d9::device_test_cooperative()
{
	// check our current status; if we lost the device, punt to GDI
	HRESULT result = m_device->TestCooperativeLevel();
	if (result == D3DERR_DEVICELOST)
		return 1;

	// if we're able to reset ourselves, try it
	if (result == D3DERR_DEVICENOTRESET)
	{
		osd_printf_verbose("Direct3D: resetting device\n");

		// free all existing resources and call reset on the device
		device_delete_resources();
		result = m_device->Reset(&m_presentation);

		// if it didn't work, punt to GDI
		if (FAILED(result))
		{
			osd_printf_error("Unable to reset, result %08lX\n", result);
			return 1;
		}

		// try to create the resources again; if that didn't work, delete the whole thing
		if (device_create_resources())
		{
			osd_printf_verbose("Direct3D: failed to recreate resources for device; failing permanently\n");
			device_delete();
			return 1;
		}
	}

	return 0;
}


//============================================================
//  config_adapter_mode
//============================================================

int renderer_d3d9::config_adapter_mode()
{
	// choose the monitor number
	m_adapter = get_adapter_for_monitor();

	// get the identifier
	D3DADAPTER_IDENTIFIER9 id;
	HRESULT result = m_d3dobj->GetAdapterIdentifier(m_adapter, 0, &id);
	if (FAILED(result))
	{
		osd_printf_error("Error getting identifier for adapter #%d\n", m_adapter);
		return 1;
	}

	osd_printf_verbose("Direct3D: Configuring adapter #%d = %s\n", m_adapter, id.Description);

	// get the current display mode
	result = m_d3dobj->GetAdapterDisplayMode(m_adapter, &m_origmode);
	if (FAILED(result))
	{
		osd_printf_error("Error getting mode for adapter #%d\n", m_adapter);
		return 1;
	}

	if (!window().fullscreen() || !video_config.switchres)
	{
		// choose a resolution: window mode case

		// bounds are from the window client rect
		RECT client;
		GetClientRectExceptMenu(dynamic_cast<win_window_info &>(window()).platform_window(), &client, window().fullscreen());
		m_width = client.right - client.left;
		m_height = client.bottom - client.top;

		// pix format is from the current mode
		m_pixformat = m_origmode.Format;
		m_refresh = 0;

		// make sure it's a pixel format we can get behind
		if (m_pixformat != D3DFMT_X1R5G5B5 && m_pixformat != D3DFMT_R5G6B5 && m_pixformat != D3DFMT_X8R8G8B8)
		{
			osd_printf_error("Device %s currently in an unsupported mode\n", window().monitor()->devicename());
			return 1;
		}
	}
	else
	{
		// choose a resolution: full screen mode case

		// default to the current mode exactly
		m_width = m_origmode.Width;
		m_height = m_origmode.Height;
		m_pixformat = m_origmode.Format;
		m_refresh = m_origmode.RefreshRate;

		// if we're allowed to switch resolutions, override with something better
		if (video_config.switchres)
			pick_best_mode();
	}

	// see if we can handle the device type
	result = m_d3dobj->CheckDeviceType(m_adapter, D3DDEVTYPE_HAL, m_pixformat, m_pixformat, !window().fullscreen());
	if (FAILED(result))
	{
		osd_printf_error("Proposed video mode not supported on device %s\n", window().monitor()->devicename());
		return 1;
	}

	return 0;
}


//============================================================
//  get_adapter_for_monitor
//============================================================

int renderer_d3d9::get_adapter_for_monitor()
{
	int maxadapter = m_d3dobj->GetAdapterCount();

	// iterate over adapters until we error or find a match
	for (int adapternum = 0; adapternum < maxadapter; adapternum++)
	{
		// get the monitor for this adapter
		HMONITOR curmonitor = m_d3dobj->GetAdapterMonitor(adapternum);

		// if we match the proposed monitor, this is it
		if (curmonitor == reinterpret_cast<HMONITOR>(window().monitor()->oshandle()))
		{
			return adapternum;
		}
	}

	// default to the default
	return D3DADAPTER_DEFAULT;
}


//============================================================
//  pick_best_mode
//============================================================

void renderer_d3d9::pick_best_mode()
{
	double target_refresh = 60.0;
	int32_t minwidth, minheight;
	float best_score = 0.0f;

	// determine the refresh rate of the primary screen
	const screen_device *primary_screen = screen_device_enumerator(window().machine().root_device()).first();
	if (primary_screen != nullptr)
	{
		target_refresh = ATTOSECONDS_TO_HZ(primary_screen->refresh_attoseconds());
	}

	// determine the minimum width/height for the selected target
	// note: technically we should not be calling this from an alternate window
	// thread; however, it is only done during init time, and the init code on
	// the main thread is waiting for us to finish, so it is safe to do so here
	window().target()->compute_minimum_size(minwidth, minheight);

	// use those as the target for now
	int32_t target_width = minwidth;
	int32_t target_height = minheight;

	// determine the maximum number of modes
	int maxmodes = m_d3dobj->GetAdapterModeCount(m_adapter, D3DFMT_X8R8G8B8);

	// enumerate all the video modes and find the best match
	osd_printf_verbose("Direct3D: Selecting video mode...\n");
	for (int modenum = 0; modenum < maxmodes; modenum++)
	{
		// check this mode
		D3DDISPLAYMODE mode;
		HRESULT result = m_d3dobj->EnumAdapterModes(m_adapter, D3DFMT_X8R8G8B8, modenum, &mode);
		if (FAILED(result))
			break;

		// skip non-32 bit modes
		if (mode.Format != D3DFMT_X8R8G8B8)
			continue;

		// compute initial score based on difference between target and current
		float size_score = 1.0f / (1.0f + fabs((float)(mode.Width - target_width)) + fabs((float)(mode.Height - target_height)));

		// if the mode is too small, give a big penalty
		if (mode.Width < minwidth || mode.Height < minheight)
			size_score *= 0.01f;

		// if mode is smaller than we'd like, it only scores up to 0.1
		if (mode.Width < target_width || mode.Height < target_height)
			size_score *= 0.1f;

		// if we're looking for a particular mode, that's a winner
		if (mode.Width == window().m_win_config.width && mode.Height == window().m_win_config.height)
			size_score = 2.0f;

		// compute refresh score
		float refresh_score = 1.0f / (1.0f + fabs((double)mode.RefreshRate - target_refresh));

		// if refresh is smaller than we'd like, it only scores up to 0.1
		if ((double)mode.RefreshRate < target_refresh)
			refresh_score *= 0.1f;

		// if we're looking for a particular refresh, make sure it matches
		if (mode.RefreshRate == window().m_win_config.refresh)
			refresh_score = 2.0f;

		// weight size and refresh equally
		float final_score = size_score + refresh_score;

		// best so far?
		osd_printf_verbose("  %4dx%4d@%3dHz -> %f\n", mode.Width, mode.Height, mode.RefreshRate, final_score * 1000.0f);
		if (final_score > best_score)
		{
			best_score = final_score;
			m_width = mode.Width;
			m_height = mode.Height;
			m_pixformat = mode.Format;
			m_refresh = mode.RefreshRate;
		}
	}
	osd_printf_verbose("Direct3D: Mode selected = %4dx%4d@%3dHz\n", m_width, m_height, m_refresh);
}


//============================================================
//  update_window_size
//============================================================

bool renderer_d3d9::update_window_size()
{
	// get the current window bounds
	auto &win = dynamic_cast<win_window_info &>(window());
	RECT client;
	GetClientRectExceptMenu(win.platform_window(), &client, window().fullscreen());

	// if we have a device and matching width/height, nothing to do
	if (m_device && rect_width(&client) == m_width && rect_height(&client) == m_height)
	{
		// clear out any pending resizing if the area didn't change
		if (win.m_resize_state == win_window_info::RESIZE_STATE_PENDING)
			win.m_resize_state = win_window_info::RESIZE_STATE_NORMAL;
		return false;
	}

	// if we're in the middle of resizing, leave it alone as well
	if (win.m_resize_state == win_window_info::RESIZE_STATE_RESIZING)
		return false;

	// set the new bounds and create the device again
	m_width = rect_width(&client);
	m_height = rect_height(&client);
	if (device_create(win.main_window()->platform_window()))
		return false;

	// reset the resize state to normal, and indicate we made a change
	win.m_resize_state = win_window_info::RESIZE_STATE_NORMAL;
	return true;
}


//============================================================
//  batch_vectors
//============================================================

void renderer_d3d9::batch_vectors(int vector_count)
{
	float quad_width = 0.0f;
	float quad_height = 0.0f;
	float target_width = 0.0f;
	float target_height = 0.0f;

	int vertex_count = vector_count * 6;
	int triangle_count = vector_count * 2;
	m_vectorbatch = mesh_alloc(vertex_count);
	m_batchindex = 0;
	uint32_t tint = 0xffffffff;

	uint32_t cached_flags = 0;
	for (render_primitive &prim : *window().m_primlist)
	{
		switch (prim.type)
		{
			case render_primitive::LINE:
				if (PRIMFLAG_GET_VECTOR(prim.flags))
				{
					batch_vector(prim);
					cached_flags = prim.flags;

					const uint8_t a = (uint8_t)std::round(prim.color.a * 255);
					const uint8_t r = (uint8_t)std::round(prim.color.r * 255);
					const uint8_t g = (uint8_t)std::round(prim.color.g * 255);
					const uint8_t b = (uint8_t)std::round(prim.color.b * 255);
					tint = (a << 24) | (b << 16) | (g << 8) | r;
				}
				break;

			case render_primitive::QUAD:
				if (PRIMFLAG_GET_VECTORBUF(prim.flags))
				{
					quad_width = prim.get_quad_width();
					quad_height = prim.get_quad_height();
					target_width = prim.get_full_quad_width();
					target_height = prim.get_full_quad_height();

					const uint8_t a = (uint8_t)std::round(prim.color.a * 255);
					const uint8_t r = (uint8_t)std::round(prim.color.r * 255);
					const uint8_t g = (uint8_t)std::round(prim.color.g * 255);
					const uint8_t b = (uint8_t)std::round(prim.color.b * 255);
					tint = (a << 24) | (b << 16) | (g << 8) | r;
				}
				break;

			default:
				// Skip
				break;
		}
	}

	// handle orientation and rotation for vectors as they were a texture
	if (m_shaders->enabled())
	{
		bool orientation_swap_xy =
			(window().machine().system().flags & ORIENTATION_SWAP_XY) == ORIENTATION_SWAP_XY;
		bool rotation_swap_xy =
			(window().target()->orientation() & ORIENTATION_SWAP_XY) == ORIENTATION_SWAP_XY;
		bool swap_xy = orientation_swap_xy ^ rotation_swap_xy;

		bool rotation_0 = window().target()->orientation() == ROT0;
		bool rotation_90 = window().target()->orientation() == ROT90;
		bool rotation_180 = window().target()->orientation() == ROT180;
		bool rotation_270 = window().target()->orientation() == ROT270;
		bool flip_x =
			((rotation_0 || rotation_270) && orientation_swap_xy) ||
			((rotation_180 || rotation_270) && !orientation_swap_xy);
		bool flip_y =
			((rotation_0 || rotation_90) && orientation_swap_xy) ||
			((rotation_180 || rotation_90) && !orientation_swap_xy);

		auto screen_width = float(this->get_width());
		auto screen_height = float(this->get_height());
		float half_screen_width = screen_width * 0.5f;
		float half_screen_height = screen_height * 0.5f;
		float screen_swap_x_factor = 1.0f / screen_width * screen_height;
		float screen_swap_y_factor = 1.0f / screen_height * screen_width;
		float screen_target_ratio_x = screen_width / target_width;
		float screen_target_ratio_y = screen_height / target_height;

		if (swap_xy)
		{
			std::swap(screen_target_ratio_x, screen_target_ratio_y);
		}

		for (int batchindex = 0; batchindex < m_batchindex; batchindex++)
		{
			if (swap_xy)
			{
				m_vectorbatch[batchindex].x *= screen_swap_x_factor;
				m_vectorbatch[batchindex].y *= screen_swap_y_factor;
				std::swap(m_vectorbatch[batchindex].x, m_vectorbatch[batchindex].y);
			}

			if (flip_x)
			{
				m_vectorbatch[batchindex].x = screen_width - m_vectorbatch[batchindex].x;
			}

			if (flip_y)
			{
				m_vectorbatch[batchindex].y = screen_height - m_vectorbatch[batchindex].y;
			}

			// center
			m_vectorbatch[batchindex].x -= half_screen_width;
			m_vectorbatch[batchindex].y -= half_screen_height;

			// correct screen/target ratio (vectors are created in screen coordinates and have to be adjusted for texture corrdinates of the target)
			m_vectorbatch[batchindex].x *= screen_target_ratio_x;
			m_vectorbatch[batchindex].y *= screen_target_ratio_y;

			// un-center
			m_vectorbatch[batchindex].x += half_screen_width;
			m_vectorbatch[batchindex].y += half_screen_height;
		}
	}

	// now add a polygon entry
	m_poly[m_numpolys].init(D3DPT_TRIANGLELIST, triangle_count, vertex_count, cached_flags, nullptr, D3DTOP_MODULATE, quad_width, quad_height, tint);
	m_numpolys++;
}

void renderer_d3d9::batch_vector(const render_primitive &prim)
{
	// get a pointer to the vertex buffer
	if (m_vectorbatch == nullptr)
	{
		return;
	}

	// compute the effective width based on the direction of the line
	float effwidth = std::max(prim.width, 2.0f);

	// determine the bounds of a quad to draw this line
	auto [b0, b1] = render_line_to_quad(prim.bounds, effwidth, effwidth);

	float lx = b1.x1 - b0.x1;
	float ly = b1.y1 - b0.y1;
	float wx = b1.x1 - b1.x0;
	float wy = b1.y1 - b1.y0;
	float line_length = sqrtf(lx * lx + ly * ly);
	float line_width = sqrtf(wx * wx + wy * wy);

	m_vectorbatch[m_batchindex + 0].x = b0.x0;
	m_vectorbatch[m_batchindex + 0].y = b0.y0;
	m_vectorbatch[m_batchindex + 1].x = b0.x1;
	m_vectorbatch[m_batchindex + 1].y = b0.y1;
	m_vectorbatch[m_batchindex + 2].x = b1.x0;
	m_vectorbatch[m_batchindex + 2].y = b1.y0;

	m_vectorbatch[m_batchindex + 3].x = b0.x1;
	m_vectorbatch[m_batchindex + 3].y = b0.y1;
	m_vectorbatch[m_batchindex + 4].x = b1.x0;
	m_vectorbatch[m_batchindex + 4].y = b1.y0;
	m_vectorbatch[m_batchindex + 5].x = b1.x1;
	m_vectorbatch[m_batchindex + 5].y = b1.y1;

	if (m_shaders->enabled())
	{
		// procedural generated texture
		m_vectorbatch[m_batchindex + 0].u0 = 0.0f;
		m_vectorbatch[m_batchindex + 0].v0 = 0.0f;
		m_vectorbatch[m_batchindex + 1].u0 = 0.0f;
		m_vectorbatch[m_batchindex + 1].v0 = 1.0f;
		m_vectorbatch[m_batchindex + 2].u0 = 1.0f;
		m_vectorbatch[m_batchindex + 2].v0 = 0.0f;

		m_vectorbatch[m_batchindex + 3].u0 = 0.0f;
		m_vectorbatch[m_batchindex + 3].v0 = 1.0f;
		m_vectorbatch[m_batchindex + 4].u0 = 1.0f;
		m_vectorbatch[m_batchindex + 4].v0 = 0.0f;
		m_vectorbatch[m_batchindex + 5].u0 = 1.0f;
		m_vectorbatch[m_batchindex + 5].v0 = 1.0f;
	}
	else
	{
		vec2f& start = get_default_texture()->get_uvstart();
		vec2f& stop = get_default_texture()->get_uvstop();

		m_vectorbatch[m_batchindex + 0].u0 = start.c.x;
		m_vectorbatch[m_batchindex + 0].v0 = start.c.y;
		m_vectorbatch[m_batchindex + 1].u0 = start.c.x;
		m_vectorbatch[m_batchindex + 1].v0 = stop.c.y;
		m_vectorbatch[m_batchindex + 2].u0 = stop.c.x;
		m_vectorbatch[m_batchindex + 2].v0 = start.c.y;

		m_vectorbatch[m_batchindex + 3].u0 = start.c.x;
		m_vectorbatch[m_batchindex + 3].v0 = stop.c.y;
		m_vectorbatch[m_batchindex + 4].u0 = stop.c.x;
		m_vectorbatch[m_batchindex + 4].v0 = start.c.y;
		m_vectorbatch[m_batchindex + 5].u0 = stop.c.x;
		m_vectorbatch[m_batchindex + 5].v0 = stop.c.y;
	}

	// determine the color of the line
	auto r = (int32_t)(prim.color.r * 255.0f);
	auto g = (int32_t)(prim.color.g * 255.0f);
	auto b = (int32_t)(prim.color.b * 255.0f);
	auto a = (int32_t)(prim.color.a * 255.0f);
	DWORD color = D3DCOLOR_ARGB(a, r, g, b);

	// set the color, Z parameters to standard values
	for (int i = 0; i < 6; i++)
	{
		m_vectorbatch[m_batchindex + i].x -= 0.5f;
		m_vectorbatch[m_batchindex + i].y -= 0.5f;
		m_vectorbatch[m_batchindex + i].z = 0.0f;
		m_vectorbatch[m_batchindex + i].rhw = 1.0f;
		m_vectorbatch[m_batchindex + i].color = color;

		// vector length/width
		m_vectorbatch[m_batchindex + i].u1 = line_length;
		m_vectorbatch[m_batchindex + i].v1 = line_width;
	}

	m_batchindex += 6;
}


//============================================================
//  draw_line
//============================================================

void renderer_d3d9::draw_line(const render_primitive &prim)
{
	// get a pointer to the vertex buffer
	vertex *vertex = mesh_alloc(4);
	if (vertex == nullptr)
	{
		return;
	}

	// compute the effective width based on the direction of the line
	float effwidth = std::max(prim.width, 1.0f);

	// determine the bounds of a quad to draw this line
	auto [b0, b1] = render_line_to_quad(prim.bounds, effwidth, 0.0f);

	vertex[0].x = b0.x0;
	vertex[0].y = b0.y0;
	vertex[1].x = b0.x1;
	vertex[1].y = b0.y1;
	vertex[2].x = b1.x0;
	vertex[2].y = b1.y0;
	vertex[3].x = b1.x1;
	vertex[3].y = b1.y1;

	vec2f& start = get_default_texture()->get_uvstart();
	vec2f& stop = get_default_texture()->get_uvstop();

	vertex[0].u0 = start.c.x;
	vertex[0].v0 = start.c.y;
	vertex[1].u0 = start.c.x;
	vertex[1].v0 = stop.c.y;
	vertex[2].u0 = stop.c.x;
	vertex[2].v0 = start.c.y;
	vertex[3].u0 = stop.c.x;
	vertex[3].v0 = stop.c.y;

	// determine the color of the line
	auto r = (int32_t)std::round(prim.color.r * 255.0f);
	auto g = (int32_t)std::round(prim.color.g * 255.0f);
	auto b = (int32_t)std::round(prim.color.b * 255.0f);
	auto a = (int32_t)std::round(prim.color.a * 255.0f);
	DWORD color = D3DCOLOR_ARGB(a, r, g, b);

	// set the color, Z parameters to standard values
	for (int i = 0; i < 4; i++)
	{
		vertex[i].z = 0.0f;
		vertex[i].rhw = 1.0f;
		vertex[i].color = color;
	}

	// now add a polygon entry
	m_poly[m_numpolys].init(D3DPT_TRIANGLESTRIP, 2, 4, prim.flags, nullptr, D3DTOP_MODULATE, 0.0f, 0.0f, (uint32_t)color);
	m_numpolys++;
}


//============================================================
//  draw_quad
//============================================================

void renderer_d3d9::draw_quad(const render_primitive &prim)
{
	texture_info *texture = m_texture_manager->find_texinfo(&prim.texture, prim.flags);
	if (texture == nullptr)
	{
		texture = get_default_texture();
	}

	// get a pointer to the vertex buffer
	vertex *vertex = mesh_alloc(4);
	if (vertex == nullptr)
	{
		return;
	}

	// fill in the vertexes clockwise
	vertex[0].x = prim.bounds.x0;
	vertex[0].y = prim.bounds.y0;
	vertex[1].x = prim.bounds.x1;
	vertex[1].y = prim.bounds.y0;
	vertex[2].x = prim.bounds.x0;
	vertex[2].y = prim.bounds.y1;
	vertex[3].x = prim.bounds.x1;
	vertex[3].y = prim.bounds.y1;
	float quad_width = prim.get_quad_width();
	float quad_height = prim.get_quad_height();

	// set the texture coordinates
	if (texture != nullptr)
	{
		vec2f& start = texture->get_uvstart();
		vec2f& stop = texture->get_uvstop();
		vec2f delta = stop - start;

		vertex[0].u0 = start.c.x + delta.c.x * prim.texcoords.tl.u;
		vertex[0].v0 = start.c.y + delta.c.y * prim.texcoords.tl.v;
		vertex[1].u0 = start.c.x + delta.c.x * prim.texcoords.tr.u;
		vertex[1].v0 = start.c.y + delta.c.y * prim.texcoords.tr.v;
		vertex[2].u0 = start.c.x + delta.c.x * prim.texcoords.bl.u;
		vertex[2].v0 = start.c.y + delta.c.y * prim.texcoords.bl.v;
		vertex[3].u0 = start.c.x + delta.c.x * prim.texcoords.br.u;
		vertex[3].v0 = start.c.y + delta.c.y * prim.texcoords.br.v;
	}

	// determine the color, allowing for over modulation
	auto r = (int32_t)std::round(prim.color.r * 255.0f);
	auto g = (int32_t)std::round(prim.color.g * 255.0f);
	auto b = (int32_t)std::round(prim.color.b * 255.0f);
	auto a = (int32_t)std::round(prim.color.a * 255.0f);
	DWORD color = D3DCOLOR_ARGB(a, r, g, b);

	// adjust half pixel X/Y offset, set the color, Z parameters to standard values
	for (int i = 0; i < 4; i++)
	{
		vertex[i].x -= 0.5f;
		vertex[i].y -= 0.5f;
		vertex[i].z = 0.0f;
		vertex[i].rhw = 1.0f;
		vertex[i].color = color;
	}

	// now add a polygon entry
	m_poly[m_numpolys].init(D3DPT_TRIANGLESTRIP, 2, 4, prim.flags, texture, D3DTOP_MODULATE, quad_width, quad_height, (uint32_t)color);
	m_numpolys++;
}


//============================================================
//  primitive_alloc
//============================================================

vertex *renderer_d3d9::mesh_alloc(int numverts)
{
	HRESULT result;

	// if we're going to overflow, flush
	if (m_lockedbuf != nullptr && m_numverts + numverts >= VERTEX_BUFFER_SIZE)
	{
		primitive_flush_pending();

		if (m_shaders->enabled())
			m_shaders->init_fsfx_quad();
	}

	// if we don't have a lock, grab it now
	if (m_lockedbuf == nullptr)
	{
		result = m_vertexbuf->Lock(0, 0, (VOID **)&m_lockedbuf, D3DLOCK_DISCARD);
		if (FAILED(result))
			return nullptr;
	}

	// if we already have the lock and enough room, just return a pointer
	if (m_lockedbuf != nullptr && m_numverts + numverts < VERTEX_BUFFER_SIZE)
	{
		int oldverts = m_numverts;
		m_numverts += numverts;
		return &m_lockedbuf[oldverts];
	}

	return nullptr;
}


//============================================================
//  primitive_flush_pending
//============================================================

void renderer_d3d9::primitive_flush_pending()
{
	// ignore if we're not locked
	if (m_lockedbuf == nullptr)
		return;

	// unlock the buffer
	HRESULT result = m_vertexbuf->Unlock();
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during vertex buffer unlock call\n", result);

	m_lockedbuf = nullptr;

	// set the stream
	result = m_device->SetStreamSource(0, m_vertexbuf.Get(), 0, sizeof(vertex));
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during device SetStreamSource call\n", result);

	m_shaders->begin_draw();

	int vertnum = 0;
	if (m_shaders->enabled())
	{
		vertnum = 6;
	}

	// now do the polys
	for (int polynum = 0; polynum < m_numpolys; polynum++)
	{
		uint32_t flags = m_poly[polynum].flags();
		texture_info *texture = m_poly[polynum].texture();
		int newfilter;

		// set the texture if different
		set_texture(texture);

		// set filtering if different
		if (texture != nullptr)
		{
			newfilter = FALSE;
			if (PRIMFLAG_GET_SCREENTEX(flags))
				newfilter = video_config.filter;

			if (m_shaders->enabled())
			{
				m_shaders->set_filter(newfilter);
			}
			else
			{
				set_filter(newfilter);
				set_wrap(PRIMFLAG_GET_TEXWRAP(flags) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP);
				set_modmode(m_poly[polynum].modmode());
			}
		}

		if (vertnum + m_poly[polynum].numverts() > m_numverts)
		{
			osd_printf_error("Error: vertnum (%d) plus poly vertex count (%d) > %d\n", vertnum, m_poly[polynum].numverts(), m_numverts);
			fflush(stdout);
		}

		assert(vertnum + m_poly[polynum].numverts() <= m_numverts);

		if(m_shaders->enabled())
		{
			m_shaders->render_quad(&m_poly[polynum], vertnum);
		}
		else
		{
			// set blend mode
			set_blendmode(PRIMFLAG_GET_BLENDMODE(flags));

			// add the primitives
			result = m_device->DrawPrimitive(m_poly[polynum].type(), vertnum, m_poly[polynum].count());
			if (FAILED(result))
				osd_printf_verbose("Direct3D: Error %08lX during device draw_primitive call\n", result);
		}

		vertnum += m_poly[polynum].numverts();
	}

	m_shaders->end_draw();

	// reset the vertex count
	m_numverts = 0;
	m_numpolys = 0;
}


std::vector<ui::menu_item> renderer_d3d9::get_slider_list()
{
	m_sliders_dirty = false;

	std::vector<ui::menu_item> sliders;
	sliders.insert(sliders.end(), m_sliders.begin(), m_sliders.end());

	if (m_shaders != nullptr && m_shaders->enabled())
	{
		std::vector<ui::menu_item> s_slider = m_shaders->get_slider_list();
		sliders.insert(sliders.end(), s_slider.begin(), s_slider.end());
	}

	return sliders;
}

void renderer_d3d9::set_sliders_dirty()
{
	m_sliders_dirty = true;
}


//============================================================
//  texture_info destructor
//============================================================

texture_info::~texture_info()
{
}


//============================================================
//  texture_info constructor
//============================================================

texture_info::texture_info(d3d_texture_manager &manager, const render_texinfo* texsource, int prescale, uint32_t flags)
	: m_texture_manager(manager)
	, m_renderer(manager.get_d3d())
	, m_hash(manager.texture_compute_hash(texsource, flags))
	, m_flags(flags)
	, m_texinfo(*texsource)
	, m_type(!PRIMFLAG_GET_SCREENTEX(flags) ? TEXTURE_TYPE_PLAIN : TEXTURE_TYPE_DYNAMIC) // required to compute texture size
	, m_xprescale(PRIMFLAG_GET_SCREENTEX(flags) ? prescale : 1)
	, m_yprescale(PRIMFLAG_GET_SCREENTEX(flags) ? prescale : 1)
{
	// compute the size
	compute_size(texsource->width, texsource->height);

	if (!PRIMFLAG_GET_SCREENTEX(flags))
	{
		// non-screen textures are easy
		assert(PRIMFLAG_TEXFORMAT(flags) != TEXFORMAT_YUY16);
		const HRESULT result = m_renderer.get_device()->CreateTexture(m_rawdims.c.x, m_rawdims.c.y, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_d3dtex, nullptr);
		if (FAILED(result))
			goto error;

		m_d3dfinaltex = m_d3dtex;
	}
	else
	{
		// screen textures are allocated differently

		// pick the format
		const DWORD usage = D3DUSAGE_DYNAMIC;
		const D3DPOOL pool = D3DPOOL_DEFAULT;
		D3DFORMAT format;
		if (PRIMFLAG_GET_TEXFORMAT(flags) == TEXFORMAT_YUY16)
			format = m_texture_manager.get_yuv_format();
		else if (PRIMFLAG_GET_TEXFORMAT(flags) == TEXFORMAT_ARGB32)
			format = D3DFMT_A8R8G8B8;
		else
			format = m_renderer.get_screen_format();

		// don't prescale above screen size
		const int maxdim = std::max(
				m_renderer.get_presentation()->BackBufferWidth,
				m_renderer.get_presentation()->BackBufferHeight);

		while ((m_xprescale > 1) && ((m_rawdims.c.x * m_xprescale) >= (2 * maxdim)))
			m_xprescale--;
		while ((m_xprescale > 1) && ((m_rawdims.c.x * m_xprescale) > manager.get_max_texture_width()))
			m_xprescale--;

		while ((m_yprescale > 1) && ((m_rawdims.c.y * m_yprescale) >= (2 * maxdim)))
			m_yprescale--;
		while ((m_yprescale > 1) && ((m_rawdims.c.y * m_yprescale) > manager.get_max_texture_height()))
			m_yprescale--;

		const int prescale = m_renderer.window().prescale();
		if ((m_xprescale != prescale) || (m_yprescale != prescale))
			osd_printf_verbose("Direct3D: adjusting prescale from %dx%d to %dx%d\n", prescale, prescale, m_xprescale, m_yprescale);

		// loop until we allocate something or error
		HRESULT result;
		for (int attempt = 0; attempt < 2; attempt++)
		{
			if ((m_xprescale == 1) && (m_yprescale == 1))
			{
				// screen textures with no prescaling are pretty easy
				result = m_renderer.get_device()->CreateTexture(m_rawdims.c.x, m_rawdims.c.y, 1, usage, format, pool, &m_d3dtex, nullptr);
				if (result == D3D_OK)
				{
					m_d3dfinaltex = m_d3dtex;
					break;
				}
			}
			else
			{
				// screen textures with prescaling require two allocations
				result = m_renderer.get_device()->CreateTexture(m_rawdims.c.x, m_rawdims.c.y, 1, usage, format, pool, &m_d3dtex, nullptr);
				if (result == D3D_OK)
				{
					// for the target surface, we allocate a render target texture
					const int scwidth = m_rawdims.c.x * m_xprescale;
					const int scheight = m_rawdims.c.y * m_yprescale;

					// target surfaces typically cannot be YCbCr, so we always pick RGB in that case
					const D3DFORMAT finalfmt = (format != m_texture_manager.get_yuv_format()) ? format : D3DFMT_A8R8G8B8;

					result = m_renderer.get_device()->CreateTexture(scwidth, scheight, 1, D3DUSAGE_RENDERTARGET, finalfmt, D3DPOOL_DEFAULT, &m_d3dfinaltex, nullptr);
					if (result == D3D_OK)
						break;

					m_d3dtex.Reset();
				}
			}

			// second attempt is always 1:1
			m_xprescale = m_yprescale = 1;
		}
		if (FAILED(result))
			goto error;
	}

	// copy the data to the texture
	assert(m_d3dtex);
	assert(m_d3dfinaltex);
	set_data(texsource, flags);

	return;

error:
	m_renderer.set_post_fx_unavailable();
	osd_printf_error("Direct3D: Critical warning: A texture failed to allocate. Expect things to get bad quickly.\n");
	m_d3dsurface.Reset();
	m_d3dtex.Reset();
}


//============================================================
//  texture_info::compute_size_subroutine
//============================================================

void texture_info::compute_size_subroutine(int texwidth, int texheight, int* p_width, int* p_height)
{
	int finalheight = texheight;
	int finalwidth = texwidth;

	// adjust the aspect ratio if we need to
	while (finalwidth < finalheight && finalheight / finalwidth > m_texture_manager.get_max_texture_aspect())
	{
		finalwidth *= 2;
	}
	while (finalheight < finalwidth && finalwidth / finalheight > m_texture_manager.get_max_texture_aspect())
	{
		finalheight *= 2;
	}

	*p_width = finalwidth;
	*p_height = finalheight;
}


//============================================================
//  texture_info::compute_size
//============================================================

void texture_info::compute_size(int texwidth, int texheight)
{
	int finalheight = texheight;
	int finalwidth = texwidth;

	m_xborderpix = 0;
	m_yborderpix = 0;

	bool shaders_enabled = m_renderer.get_shaders()->enabled();
	bool wrap_texture = (m_flags & PRIMFLAG_TEXWRAP_MASK) == PRIMFLAG_TEXWRAP_MASK;

	// skip border when shaders are enabled
	if (!shaders_enabled)
	{
		// if we're not wrapping, add a 1-2 pixel border on all sides
		if (!wrap_texture)
		{
			// note we need 2 pixels in X for YUY textures
			//m_xborderpix = (PRIMFLAG_GET_TEXFORMAT(m_flags) == TEXFORMAT_YUY16) ? 2 : 1;
			//m_yborderpix = 1;
		}
	}

	finalwidth += 2 * m_xborderpix;
	finalheight += 2 * m_yborderpix;

	// take texture size as given when shaders are enabled
	if (!shaders_enabled)
	{
		compute_size_subroutine(finalwidth, finalheight, &finalwidth, &finalheight);

		// if we added pixels for the border, and that just barely pushed us over, take it back
		if (finalwidth > m_texture_manager.get_max_texture_width() || finalheight > m_texture_manager.get_max_texture_height())
		{
			finalheight = texheight;
			finalwidth = texwidth;

			m_xborderpix = 0;
			m_yborderpix = 0;

			compute_size_subroutine(finalwidth, finalheight, &finalwidth, &finalheight);
		}
	}

	// if we're above the max width/height, do what?
	if (finalwidth > m_texture_manager.get_max_texture_width() || finalheight > m_texture_manager.get_max_texture_height())
	{
		static bool printed = false;
		if (!printed)
		{
			osd_printf_warning(
					"Texture too big! (wanted: %dx%d, max is %dx%d)\n",
					finalwidth,
					finalheight,
					m_texture_manager.get_max_texture_width(),
					m_texture_manager.get_max_texture_height());
		}
		printed = true;
	}

	// compute the U/V scale factors
	m_start.c.x = (float)m_xborderpix / (float)finalwidth;
	m_start.c.y = (float)m_yborderpix / (float)finalheight;
	m_stop.c.x = (float)(texwidth + m_xborderpix) / (float)finalwidth;
	m_stop.c.y = (float)(texheight + m_yborderpix) / (float)finalheight;

	// set the final values
	m_rawdims.c.x = finalwidth;
	m_rawdims.c.y = finalheight;
}


//============================================================
//  copyline_palette16
//============================================================

inline void texture_info::copyline_palette16(uint32_t *dst, const uint16_t *src, int width, const rgb_t *palette, int xborderpix)
{
	if (xborderpix)
		*dst++ = 0xff000000 | palette[*src];
	for (int x = 0; x < width; x++)
		*dst++ = 0xff000000 | palette[*src++];
	if (xborderpix)
		*dst++ = 0xff000000 | palette[*--src];
}


//============================================================
//  copyline_rgb32
//============================================================

inline void texture_info::copyline_rgb32(uint32_t *dst, const uint32_t *src, int width, const rgb_t *palette, int xborderpix)
{
	if (palette != nullptr)
	{
		if (xborderpix)
		{
			rgb_t srcpix = *src;
			*dst++ = 0xff000000 | palette[0x200 + srcpix.r()] | palette[0x100 + srcpix.g()] | palette[srcpix.b()];
		}
		for (int x = 0; x < width; x++)
		{
			rgb_t srcpix = *src++;
			*dst++ = 0xff000000 | palette[0x200 + srcpix.r()] | palette[0x100 + srcpix.g()] | palette[srcpix.b()];
		}
		if (xborderpix)
		{
			rgb_t srcpix = *--src;
			*dst++ = 0xff000000 | palette[0x200 + srcpix.r()] | palette[0x100 + srcpix.g()] | palette[srcpix.b()];
		}
	}
	else
	{
		if (xborderpix)
			*dst++ = 0xff000000 | *src;
		for (int x = 0; x < width; x++)
			*dst++ = 0xff000000 | *src++;
		if (xborderpix)
			*dst++ = 0xff000000 | *--src;
	}
}


//============================================================
//  copyline_argb32
//============================================================

inline void texture_info::copyline_argb32(uint32_t *dst, const uint32_t *src, int width, const rgb_t *palette, int xborderpix)
{
	if (palette != nullptr)
	{
		if (xborderpix)
		{
			rgb_t srcpix = *src;
			*dst++ = (srcpix & 0xff000000) | palette[0x200 + srcpix.r()] | palette[0x100 + srcpix.g()] | palette[srcpix.b()];
		}
		for (int x = 0; x < width; x++)
		{
			rgb_t srcpix = *src++;
			*dst++ = (srcpix & 0xff000000) | palette[0x200 + srcpix.r()] | palette[0x100 + srcpix.g()] | palette[srcpix.b()];
		}
		if (xborderpix)
		{
			rgb_t srcpix = *--src;
			*dst++ = (srcpix & 0xff000000) | palette[0x200 + srcpix.r()] | palette[0x100 + srcpix.g()] | palette[srcpix.b()];
		}
	}
	else
	{
		if (xborderpix)
			*dst++ = *src;
		memcpy(dst, src, sizeof(uint32_t) * width);
		dst += width;
		src += width;
		if (xborderpix)
			*dst++ = *--src;
	}
}


//============================================================
//  copyline_yuy16_to_yuy2
//============================================================

inline void texture_info::copyline_yuy16_to_yuy2(uint16_t *dst, const uint16_t *src, int width, const rgb_t *palette)
{
	assert(width % 2 == 0);

	if (palette != nullptr) // palette (really RGB map) case
	{
		for (int x = 0; x < width; x += 2)
		{
			uint16_t srcpix0 = *src++;
			uint16_t srcpix1 = *src++;
			*dst++ = palette[0x000 + (srcpix0 >> 8)] | (srcpix0 << 8);
			*dst++ = palette[0x000 + (srcpix1 >> 8)] | (srcpix1 << 8);
		}
	}
	else // direct case
	{
		for (int x = 0; x < width; x += 2)
		{
			uint16_t srcpix0 = *src++;
			uint16_t srcpix1 = *src++;
			*dst++ = (srcpix0 >> 8) | (srcpix0 << 8);
			*dst++ = (srcpix1 >> 8) | (srcpix1 << 8);
		}
	}
}


//============================================================
//  copyline_yuy16_to_uyvy
//============================================================

inline void texture_info::copyline_yuy16_to_uyvy(uint16_t *dst, const uint16_t *src, int width, const rgb_t *palette)
{
	assert(width % 2 == 0);

	if (palette != nullptr) // palette (really RGB map) case
	{
		for (int x = 0; x < width; x += 2)
		{
			uint16_t srcpix0 = *src++;
			uint16_t srcpix1 = *src++;
			*dst++ = palette[0x100 + (srcpix0 >> 8)] | (srcpix0 & 0xff);
			*dst++ = palette[0x100 + (srcpix1 >> 8)] | (srcpix1 & 0xff);
		}
	}

	// direct case
	else
	{
		memcpy(dst, src, sizeof(uint16_t) * width);
	}
}


//============================================================
//  copyline_yuy16_to_argb
//============================================================

inline void texture_info::copyline_yuy16_to_argb(uint32_t *dst, const uint16_t *src, int width, const rgb_t *palette)
{
	assert(width % 2 == 0);

	if (palette != nullptr) // palette (really RGB map) case
	{
		for (int x = 0; x < width / 2; x++)
		{
			uint16_t srcpix0 = *src++;
			uint16_t srcpix1 = *src++;
			uint8_t cb = srcpix0 & 0xff;
			uint8_t cr = srcpix1 & 0xff;
			*dst++ = ycc_to_rgb(palette[0x000 + (srcpix0 >> 8)], cb, cr);
			*dst++ = ycc_to_rgb(palette[0x000 + (srcpix1 >> 8)], cb, cr);
		}
	}
	else // direct case
	{
		for (int x = 0; x < width; x += 2)
		{
			uint16_t srcpix0 = *src++;
			uint16_t srcpix1 = *src++;
			uint8_t cb = srcpix0 & 0xff;
			uint8_t cr = srcpix1 & 0xff;
			*dst++ = ycc_to_rgb(srcpix0 >> 8, cb, cr);
			*dst++ = ycc_to_rgb(srcpix1 >> 8, cb, cr);
		}
	}
}


//============================================================
//  texture_set_data
//============================================================

void texture_info::set_data(const render_texinfo *texsource, uint32_t flags)
{
	D3DLOCKED_RECT rect;
	HRESULT result;

	// lock the texture
	switch (m_type)
	{
		default:
		case TEXTURE_TYPE_PLAIN:    result = m_d3dtex->LockRect(0, &rect, nullptr, 0);                 break;
		case TEXTURE_TYPE_DYNAMIC:  result = m_d3dtex->LockRect(0, &rect, nullptr, D3DLOCK_DISCARD);   break;
		case TEXTURE_TYPE_SURFACE:  result = m_d3dsurface->LockRect(&rect, nullptr, D3DLOCK_DISCARD);  break;
	}
	if (FAILED(result))
	{
		return;
	}

	// loop over Y
	int tex_format = PRIMFLAG_GET_TEXFORMAT(flags);
#if 0
	if (tex_format == TEXFORMAT_ARGB32 && texsource->palette == nullptr && texsource->width == texsource->rowpixels && m_xborderpix == 0 && m_yborderpix == 0)
	{
		memcpy((BYTE *)rect.pBits, texsource->base, sizeof(uint32_t) * texsource->width * texsource->height);
	}
	else
#endif
	{
		int miny = 0 - m_yborderpix;
		int maxy = texsource->height + m_yborderpix;

		for (int dsty = miny; dsty < maxy; dsty++)
		{
			int srcy = (dsty < 0) ? 0 : (dsty >= texsource->height) ? texsource->height - 1 : dsty;

			void *dst = (BYTE *)rect.pBits + (dsty + m_yborderpix) * rect.Pitch;

			switch (tex_format)
			{
				case TEXFORMAT_PALETTE16:
					copyline_palette16((uint32_t *)dst, (uint16_t *)texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, m_xborderpix);
					break;

				case TEXFORMAT_RGB32:
					copyline_rgb32((uint32_t *)dst, (uint32_t *)texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, m_xborderpix);
					break;

				case TEXFORMAT_ARGB32:
					copyline_argb32((uint32_t *)dst, (uint32_t *)texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, m_xborderpix);
					break;

				case TEXFORMAT_YUY16:
					if (m_texture_manager.get_yuv_format() == D3DFMT_YUY2)
						copyline_yuy16_to_yuy2((uint16_t *)dst, (uint16_t *)texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette);
					else if (m_texture_manager.get_yuv_format() == D3DFMT_UYVY)
						copyline_yuy16_to_uyvy((uint16_t *)dst, (uint16_t *)texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette);
					else
						copyline_yuy16_to_argb((uint32_t *)dst, (uint16_t *)texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette);
					break;

				default:
					osd_printf_error("Unknown texture blendmode=%d format=%d\n", PRIMFLAG_GET_BLENDMODE(flags), PRIMFLAG_GET_TEXFORMAT(flags));
					break;
			}
		}
	}

	// unlock
	switch (m_type)
	{
		default:
		case TEXTURE_TYPE_PLAIN:    result = m_d3dtex->UnlockRect(0);   break;
		case TEXTURE_TYPE_DYNAMIC:  result = m_d3dtex->UnlockRect(0);   break;
		case TEXTURE_TYPE_SURFACE:  result = m_d3dsurface->UnlockRect();  break;
	}
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during texture UnlockRect call\n", result);

	// prescale
	prescale();
}


//============================================================
//  texture_info::prescale
//============================================================

void texture_info::prescale()
{
	// if we don't need to, just skip it
	if (m_d3dtex.Get() == m_d3dfinaltex.Get())
	{
		assert(m_xprescale == 1);
		assert(m_yprescale == 1);
		return;
	}

	assert(m_d3dtex);
	assert((m_xprescale > 1) || (m_yprescale > 1));
	HRESULT result;

	// for all cases, we need to get the surface of the render target
	Microsoft::WRL::ComPtr<IDirect3DSurface9> scale_surface;
	result = m_d3dfinaltex->GetSurfaceLevel(0, &scale_surface);
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during texture GetSurfaceLevel call\n", result);

	// if we have an offscreen plain surface, we can just StretchRect to it

	// first remember the original render target and set the new one
	Microsoft::WRL::ComPtr<IDirect3DSurface9> backbuffer;
	result = m_renderer.get_device()->GetRenderTarget(0, &backbuffer);
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during device GetRenderTarget call\n", result);
	result = m_renderer.get_device()->SetRenderTarget(0, scale_surface.Get());
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during device SetRenderTarget call 1\n", result);
	m_renderer.reset_render_states();

	// start the scene
	result = m_renderer.get_device()->BeginScene();
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during device BeginScene call\n", result);

	// configure the rendering pipeline
	m_renderer.set_filter(FALSE);
	m_renderer.set_blendmode(BLENDMODE_NONE);
	result = m_renderer.get_device()->SetTexture(0, m_d3dtex.Get());
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during device SetTexture call\n", result);

	// lock the vertex buffer
	vertex *lockedbuf;
	result = m_renderer.get_vertex_buffer()->Lock(0, 0, (VOID **)&lockedbuf, D3DLOCK_DISCARD);
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during vertex buffer lock call\n", result);

	// configure the X/Y coordinates on the target surface
	lockedbuf[0].x = -0.5f;
	lockedbuf[0].y = -0.5f;
	lockedbuf[1].x = (float)((m_texinfo.width + 2 * m_xborderpix) * m_xprescale) - 0.5f;
	lockedbuf[1].y = -0.5f;
	lockedbuf[2].x = -0.5f;
	lockedbuf[2].y = (float)((m_texinfo.height + 2 * m_yborderpix) * m_yprescale) - 0.5f;
	lockedbuf[3].x = (float)((m_texinfo.width + 2 * m_xborderpix) * m_xprescale) - 0.5f;
	lockedbuf[3].y = (float)((m_texinfo.height + 2 * m_yborderpix) * m_yprescale) - 0.5f;

	// configure the U/V coordintes on the source texture
	lockedbuf[0].u0 = 0.0f;
	lockedbuf[0].v0 = 0.0f;
	lockedbuf[1].u0 = (float)(m_texinfo.width + 2 * m_xborderpix) / (float)m_rawdims.c.x;
	lockedbuf[1].v0 = 0.0f;
	lockedbuf[2].u0 = 0.0f;
	lockedbuf[2].v0 = (float)(m_texinfo.height + 2 * m_yborderpix) / (float)m_rawdims.c.y;
	lockedbuf[3].u0 = (float)(m_texinfo.width + 2 * m_xborderpix) / (float)m_rawdims.c.x;
	lockedbuf[3].v0 = (float)(m_texinfo.height + 2 * m_yborderpix) / (float)m_rawdims.c.y;

	// reset the remaining vertex parameters
	for (int i = 0; i < 4; i++)
	{
		lockedbuf[i].z = 0.0f;
		lockedbuf[i].rhw = 1.0f;
		lockedbuf[i].color = D3DCOLOR_ARGB(0xff,0xff,0xff,0xff);
	}

	// unlock the vertex buffer
	result = m_renderer.get_vertex_buffer()->Unlock();
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during vertex buffer unlock call\n", result);

	// set the stream and draw the triangle strip
	result = m_renderer.get_device()->SetStreamSource(0, m_renderer.get_vertex_buffer(), 0, sizeof(vertex));
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during device SetStreamSource call\n", result);
	result = m_renderer.get_device()->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during device DrawPrimitive call\n", result);

	// end the scene
	result = m_renderer.get_device()->EndScene();
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during device end_scene call\n", result);

	// reset the render target and release our reference to the backbuffer
	result = m_renderer.get_device()->SetRenderTarget(0, backbuffer.Get());
	if (FAILED(result))
		osd_printf_verbose("Direct3D: Error %08lX during device SetRenderTarget call 2\n", result);
	backbuffer.Reset();
	m_renderer.reset_render_states();

	// release our reference to the target surface
	scale_surface.Reset();
}


//============================================================
//  d3d_render_target::~d3d_render_target
//============================================================

d3d_render_target::~d3d_render_target()
{
}


//============================================================
//  d3d_render_target::init - initializes a render target
//============================================================

bool d3d_render_target::init(renderer_d3d9 *d3d, int source_width, int source_height, int target_width, int target_height, int screen_index)
{
	HRESULT result;

	this->width = source_width;
	this->height = source_height;

	this->target_width = target_width;
	this->target_height = target_height;

	this->screen_index = screen_index;

	for (int index = 0; index < 2; index++)
	{
		result = d3d->get_device()->CreateTexture(source_width, source_height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &source_texture[index], nullptr);
		if (FAILED(result))
			return false;

		source_texture[index]->GetSurfaceLevel(0, &source_surface[index]);

		result = d3d->get_device()->CreateTexture(target_width, target_height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &target_texture[index], nullptr);
		if (FAILED(result))
			return false;

		target_texture[index]->GetSurfaceLevel(0, &target_surface[index]);
	}

	result = d3d->get_device()->CreateTexture(target_width, target_height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &cache_texture, nullptr);
	if (FAILED(result))
		return false;

	cache_texture->GetSurfaceLevel(0, &cache_surface);

	const screen_device *first_screen = screen_device_enumerator(d3d->window().machine().root_device()).first();
	bool vector_screen =
		first_screen != nullptr &&
		first_screen->screen_type() == SCREEN_TYPE_VECTOR;

	float scale_factor = 0.75f;
	int scale_count = vector_screen ? MAX_BLOOM_COUNT : HALF_BLOOM_COUNT;

	auto bloom_width = (float)source_width;
	auto bloom_height = (float)source_height;
	float bloom_size = bloom_width < bloom_height ? bloom_width : bloom_height;
	for (int bloom_index = 0; bloom_index < scale_count && bloom_size >= 2.0f; bloom_size *= scale_factor)
	{
		this->bloom_dims[bloom_index][0] = (int)bloom_width;
		this->bloom_dims[bloom_index][1] = (int)bloom_height;

		result = d3d->get_device()->CreateTexture((int)bloom_width, (int)bloom_height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &bloom_texture[bloom_index], nullptr);
		if (FAILED(result))
			return false;

		bloom_texture[bloom_index]->GetSurfaceLevel(0, &bloom_surface[bloom_index]);

		bloom_width *= scale_factor;
		bloom_height *= scale_factor;

		bloom_index++;

		this->bloom_count = bloom_index;
	}

	return true;
}

texture_info *renderer_d3d9::get_default_texture()
{
	return m_texture_manager->get_default_texture();
}

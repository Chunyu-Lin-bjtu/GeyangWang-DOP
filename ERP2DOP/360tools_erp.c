/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2016, Samsung Electronics Co., Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of Samsung Electronics Co., Ltd. nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <math.h>
#include "360tools.h"
#include "360tools_erp.h"

static void erp_to_cpp_plane(int w_src, int h_src, int s_src, void * src,
	int w_dst, int h_dst, void * dst, int s_dst, int cs, int opt, int pad_sz)
{
	//ERP To DOP
	void(*fn_resample)(void * src, int w_start, int w_end, int h_start, int h_end, \
		int s_src, double x, double y, void * dst, int x_dst);
	double x, y, phi;
	int    i, j;
	uint8 * octagonmap;//°Ë±ßÐÎmap
	int     w_start, w_end, h_start;

	//int octagonw = w_src / 5 * 4 * 2, octagonh = h_src / 5 * 4;
	int octagonw = w_src, octagonh = h_src;
	octagonmap = (uint8 *)s360_malloc(sizeof(uint8) * octagonw * octagonh);
	s360_mset(octagonmap, 0, octagonw * octagonh);

	w_start = opt ? -pad_sz : 0;
	w_end = opt ? octagonw + pad_sz : octagonw;
	h_start = 0;

	int edgehw = octagonw / 16;
	for (j = 0; j < octagonh * 3 / 8; j++)
	{
		s360_mset(octagonmap + j * octagonw + edgehw * 3 - j, 1, edgehw * 2 + j * 2);
		s360_mset(octagonmap + j * octagonw + edgehw * 11 - 1 - j, 1, edgehw * 2 + (j + 1) * 2);
		s360_mset(octagonmap + (octagonh - j - 1)* octagonw + edgehw * 3 - j, 1, edgehw * 2 + j * 2);
		s360_mset(octagonmap + (octagonh - j - 1)* octagonw + edgehw * 11 - 1 - j, 1, edgehw * 2 + (j + 1) * 2);
	}
	for (j = octagonh * 3 / 8; j < octagonh / 2; j++)
	{
		s360_mset(octagonmap + j * octagonw, 1, octagonw);
		s360_mset(octagonmap + (octagonh - j - 1)* octagonw, 1, octagonw);
	}
	
	if (cs == S360_COLORSPACE_YUV420)
	{
		fn_resample = resample_2d;
	}
	else if (cs == S360_COLORSPACE_YUV420_10)
	{
		fn_resample = resample_2d_10b;
		s_dst <<= 1;
	}

	for (j = 0; j<octagonh; j++)
	{
		y = (double)j / octagonh * h_src;
		//phi = 3 * asin((double)j / h_dst - 0.5);
		//y = octagonh * (phi + PI / 2) / PI;

		int c = 0;

		int octagonnum = 0;
		if (j >= octagonh * 3 / 8 && j < octagonh * 5 / 8)
			octagonnum = octagonw;
		else if (j < octagonh * 3 / 8)
			octagonnum = edgehw * 4 + j * 4 + 2;
		else
			octagonnum = edgehw * 4 + (octagonh - j - 1) * 4 + 2;

		for (i = 0; i<octagonw; i++)
		{
			if (octagonmap[j*octagonw + i] == 1)
			{
				if (octagonnum == octagonw)
				{
					if (c < (double)octagonnum * 3 / 4)
						x = (double)c / octagonnum * w_src + (double)w_src / 4;
					else if (c >= (double)octagonnum / 4 * 3)
						x = (c - (double)octagonnum / 4 * 3) / octagonnum * w_src;
				}
				else
				{
					if (c < (double)octagonnum * 3 / 4 - 0.5)
						x = (double)(c + 0.5) / octagonnum * w_src + (double)w_src / 4;
					else if (c >= (double)octagonnum / 4 * 3 - 0.5)
						x = (c - (double)octagonnum / 4 * 3 + 0.5) / octagonnum * w_src;
				}

				fn_resample(src, w_start, w_end, h_start, octagonh, s_src, x, y, dst, i);
				c++;
			}
		}
		dst = (void *)((uint8 *)dst + s_dst);
	}
	
	s360_mfree(octagonmap);
}

int s360_erp_to_cpp(S360_IMAGE * img_src, S360_IMAGE * img_dst, int opt, S360_MAP * map)
{
	int i, w_src, w_dst, h_src, h_dst, pad_sz;

	s360_img_reset(img_dst);
	if (opt & S360_OPT_PAD)
		s360_pad_erp(img_src);

	if((img_src->colorspace == S360_COLORSPACE_YUV420) || ((img_src->colorspace == S360_COLORSPACE_YUV420_10)))
	{
		for(i=0; i<3; i++)
		{
			if(i == 0)
			{
				w_src = img_src->width;
				w_dst = img_dst->width;
				h_src = img_src->height;
				h_dst = img_dst->height;
				pad_sz = (opt & S360_OPT_PAD) ? PAD_SIZE : 0;
			}
			else
			{
				w_src = (img_src->width + 1)  >> 1;
				w_dst = (img_dst->width + 1)  >> 1;
				h_src = (img_src->height + 1) >> 1;
				h_dst = (img_dst->height + 1) >> 1;
				pad_sz = (opt & S360_OPT_PAD) ? PAD_SIZE >> 1 : 0;
			}
			erp_to_cpp_plane(w_src, h_src, img_src->stride[i], \
				img_src->buffer[i], w_dst, h_dst, img_dst->buffer[i], \
				img_dst->stride[i], img_src->colorspace, opt, pad_sz);
		}
	}
	else
	{
		return S360_ERR_UNSUPPORTED_COLORSPACE;
	}
	return S360_OK;
}

static void cpp_to_erp_plane(int w_src, int h_src, int s_src, void * src, \
	int w_dst, int h_dst, void * octagon2compact, void * dst, int s_dst, int cs)
{
	resample_fn fn_resample;
	uint8 * octagonmap;//°Ë±ßÐÎmap
	uint8 * octagon2compactmap;
	uint16 * octagon2compactdst = (uint16 *)octagon2compact;
	int     i, j;
	double  phi;
	double  x, y;

	fn_resample = resample_fp(cs);

	//int octagonw = w_src / 2, octagonh = h_src / 2;
	int octagonw = w_src, octagonh = h_src;
	octagonmap = (uint8 *)s360_malloc(sizeof(uint8) * octagonw * octagonh);
	s360_mset(octagonmap, 0, octagonw * octagonh);
	octagon2compactmap = (uint8 *)s360_malloc(sizeof(uint8) * octagonw * octagonh);
	s360_mset(octagon2compactmap, 0, octagonw * octagonh);

	int edgehw = octagonw / 16;
	for (j = 0; j < octagonh * 3 / 8; j++)
	{
		s360_mset(octagonmap + j * octagonw + edgehw * 3 - j, 1, edgehw * 2 + j * 2);
		s360_mset(octagonmap + j * octagonw + edgehw * 11 - 1 - j, 1, edgehw * 2 + (j + 1) * 2);
		s360_mset(octagonmap + (octagonh - j - 1)* octagonw + edgehw * 3 - j, 1, edgehw * 2 + j * 2);
		s360_mset(octagonmap + (octagonh - j - 1)* octagonw + edgehw * 11 - 1 - j, 1, edgehw * 2 + (j + 1) * 2);
	}
	for (j = octagonh * 3 / 8; j < octagonh / 2; j++)
	{
		s360_mset(octagonmap + j * octagonw, 1, octagonw);
		s360_mset(octagonmap + (octagonh - j - 1)* octagonw, 1, octagonw);
	}
	for (j = 0; j < octagonh * 3 / 8; j++)
	{
		s360_mset(octagon2compactmap + j * octagonw + octagonw / 2 - (edgehw * 2 + j * 2 + 1), 1, edgehw * 4 + j * 4 + 2);
		s360_mset(octagon2compactmap + (octagonh - j - 1)* octagonw + octagonw / 2 - (edgehw * 2 + j * 2 + 1), 1, edgehw * 4 + j * 4 + 2);
	}
	for (j = octagonh * 3 / 8; j < octagonh / 2; j++)
	{
		s360_mset(octagon2compactmap + j * octagonw, 1, octagonw);
		s360_mset(octagon2compactmap + (octagonh - j - 1)* octagonw, 1, octagonw);
	}

	uint16 * src16 = (uint16 *)src;
	for (j = 0; j<octagonh; j++)
	{
		int c = 0;

		int octagonnum = 0;
		if (j >= octagonh * 3 / 8 && j < octagonh * 5 / 8)
			octagonnum = octagonw;
		else if (j < octagonh * 3 / 8)
			octagonnum = edgehw * 4 + j * 4 + 2;
		else
			octagonnum = edgehw * 4 + (octagonh - j - 1) * 4 + 2;

		int semil = 0, semir = 0;
		if (octagonnum == octagonw)
		{
			semil = octagonnum / 4;
			semir = octagonnum * 3 / 4;
		}
		else
		{
			semil = (octagonnum / 2 - 1) / 2;
			semir = semil + octagonnum / 2;
		}

		for (i = 0; i<octagonw; i++)
		{
			if (octagonmap[j*octagonw + i] == 1)
			{
				int idx = 0, idy = 0;
				if (c < semir)
					octagon2compactdst[octagonw / 2 - semil + c] = src16[i];
				else
					octagon2compactdst[octagonw / 2 - octagonnum / 2 + c - semir] = src16[i];
				c++;
			}
		}
		octagon2compactdst = octagon2compactdst + s_src;
		src16 = src16 + s_src;
	}
	
	if (cs == S360_COLORSPACE_YUV420)
	{
		pad_cpp_plane_circular((uint8 *)octagon2compact, octagonw, octagonh, s_src, octagon2compactmap);
	}
	else if (cs == S360_COLORSPACE_YUV420_10)
	{
		pad_cpp_plane_circular_10b((uint16 *)octagon2compact, octagonw, octagonh, s_src, octagon2compactmap);
	}

	if (cs == S360_COLORSPACE_YUV420_10)
	{
		s_dst <<= 1;
	}

	for (j = 0; j<h_dst; j++)
	{
		int c = 0;
		y = (double)j / h_dst * octagonh;
		//y = h_src * (.5 + sin(PI / 3 * ((double)j / octagonh - .5f)));

		double octagonnum = 0;
		if (y >= octagonh * 3 / 8 && y < octagonh * 5 / 8)
			octagonnum = octagonw;
		else if (y < octagonh * 3 / 8)
			octagonnum = (edgehw * 4 + y * 4 + 2) >= octagonw ? octagonw : edgehw * 4 + y * 4 + 2;
		else
			octagonnum = (edgehw * 4 + (octagonh - y - 1) * 4 + 2) >= octagonw ? octagonw : edgehw * 4 + (octagonh - y - 1) * 4 + 2;

		double octagon2compactleft = 0;
		int k = 0;
		while (octagon2compactmap[k + (int)y*w_src] == 0 && k < w_src) k++;
		octagon2compactleft = k;
		
		if (y >= octagonh * 3 / 8 && y < octagonh * 5 / 8)
			octagon2compactleft = 0;
		else if (y < octagonh * 3 / 8)
			octagon2compactleft = (octagonw / 2 - edgehw * 2 - 1 - y * 2) >= 0 ? octagonw / 2 - edgehw * 2 - 1 - y * 2 : 0;
		else
			octagon2compactleft = (octagonw / 2 - edgehw * 2 - 1 - (octagonh - y - 1) * 2) >= 0 ? octagonw / 2 - edgehw * 2 - 1 - (octagonh - y - 1) * 2 : 0;
		octagonnum = octagonw - 2 * octagon2compactleft;

		for (i = 0; i<w_dst; i++)
		{
			x = (double)i / w_dst * octagonnum + octagon2compactleft;
			
			fn_resample(octagon2compact, 0, w_src, 0, h_src, s_src, x, y, dst, i);
		}
		dst = (void *)((uint8 *)dst + s_dst);
	}
	
	s360_mfree(octagonmap);
	s360_mfree(octagon2compactmap);

}

int s360_cpp_to_erp(S360_IMAGE * img_src, S360_IMAGE * img_dst, int opt, S360_MAP * map)
{
	int i, w_src, w_dst, h_src, h_dst;

	S360_IMAGE * octagon2compact = s360_img_create(img_src->width / 5 * 4 * 2, img_src->height / 5 * 4, img_src->colorspace, opt);
	s360_img_reset(octagon2compact);

	if(IS_VALID_CS(img_src->colorspace))
	{
		for(i=0; i<3; i++)
		{
			if(i == 0)
			{
				w_src = img_src->width;
				h_src = img_src->height;
				w_dst = img_dst->width;
				h_dst = img_dst->height;
			}
			else
			{
				w_src = img_src->width  >> 1;
				h_src = img_src->height >> 1;
				w_dst = img_dst->width  >> 1;
				h_dst = img_dst->height >> 1;
			}
			cpp_to_erp_plane(w_src, h_src, img_src->stride[i], \
				img_src->buffer[i], w_dst, h_dst, octagon2compact->buffer[i], img_dst->buffer[i], \
				img_dst->stride[i], img_src->colorspace);
			//s360_mcpy(img_dst->buffer[i], octagon2compact->buffer[i], img_dst->stride[i] * img_dst->elevation[i] * sizeof(uint16));
		}
		return S360_OK;
	}
	else
	{
		return S360_ERR_UNSUPPORTED_COLORSPACE;
	}
}

static void pad_erp_plane(uint8 * buf, int w, int h, int s, int pad_sz)
{
	uint8     * src;
	uint8     * dst;
	int         i;

	// Pad Left
	src = buf + w - pad_sz;
	dst = buf - pad_sz;
	for(i=0;i<h;i++)
	{
		memcpy(dst, src, pad_sz*sizeof(uint8));
		dst += s;
		src += s;
	}

	// Pad Right
	src = buf;
	dst = buf + w;

	for(i=0;i<h;i++)
	{
		memcpy(dst, src, pad_sz*sizeof(uint8));
		dst += s;
		src += s;
	}
}

static void pad_erp_plane_10b(uint16 * buf, int w, int h, int s, int pad_sz)
{
	uint16     * src;
	uint16     * dst;
	int          i;

	// Pad Left
	src = buf + w - pad_sz;
	dst = buf - pad_sz;
	for (i=0;i<h;i++)
	{
		memcpy(dst, src, pad_sz*sizeof(uint16));
		dst += s;
		src += s;
	}

	// Pad Right
	src = buf;
	dst = buf + w;

	for (i=0;i<h;i++)
	{
		memcpy(dst, src, pad_sz*sizeof(uint16));
		dst += s;
		src += s;
	}
}

int s360_pad_erp(S360_IMAGE * img)
{
	int i, w, h;
	int pad_sz;

	if(IS_VALID_CS(img->colorspace))
	{
		for(i=0;i<3;i++)
		{
			if(i == 0)
			{
				w = img->width;
				h = img->height;
				pad_sz = PAD_SIZE;
			}
			else
			{
				w = img->width >> 1;
				h = img->height >> 1;
				pad_sz = PAD_SIZE >> 1;
			}

			if (img->colorspace == S360_COLORSPACE_YUV420)
			{
				pad_erp_plane((uint8 *)(img->buffer[i]), w, h, img->stride[i], pad_sz);
			}
			else if (img->colorspace == S360_COLORSPACE_YUV420_10)
			{
				pad_erp_plane_10b((uint16 *)(img->buffer[i]), w, h, img->stride[i], pad_sz);
			}
		}
	}
	else
	{
		return S360_ERR_UNSUPPORTED_COLORSPACE;
	}
	return S360_OK;
}
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
	void(*fn_resample)(void * src, int w_start, int w_end, int h_start, int h_end, \
		int s_src, double x, double y, void * dst, int x_dst);
	//2octagon To CPP
	double x, y, phi;
	int    i, j;
	void * dst0 = dst;
	uint8 * map2CPP;//2CPPmap
	uint8 * map;//CPPmap
	uint8 * octagonmap;//??????map
	int     w_start, w_end, h_start;

	w_start = opt ? -pad_sz : 0;
	w_end = opt ? w_src + pad_sz : w_src;
	h_start = 0;

	map2CPP = (uint8 *)s360_malloc(sizeof(uint8) * w_src * h_src);
	map = (uint8 *)s360_malloc(sizeof(uint8) * w_src * h_src);
	cpp_map_plane(w_src, h_src, s_dst, map2CPP);
	CPP_map_plane(w_src, h_src, s_dst, map);
	octagonmap = (uint8 *)s360_malloc(sizeof(uint8) * w_src * h_src);
	s360_mset(octagonmap, 0, w_src * h_src);

	//????????????map
	int lcons45lean = 0, lcons45leantemp = 0;
	int lastleftedg = 0;
	int ltangpointy = 0, ltangpointx = 0;
	int leftedg;
	for (j = 0; j < (h_src >> 1); j++)
	{
		for (i = 0; i < w_src; i++)
		{
			if (map2CPP[i + j*w_src] == 1)
			{
				leftedg = i;
				break;
			}
		}

		if (leftedg == lastleftedg - 1)
			lcons45leantemp++;
		else
		{
			if (lcons45lean < lcons45leantemp)
			{
				lcons45lean = lcons45leantemp;
				ltangpointy = j - 1;
				ltangpointx = lastleftedg;
			}
			lcons45leantemp = 0;
		}
		lastleftedg = leftedg;
	}

	for (j = 0; j < (h_src >> 1); j++)
	{
		leftedg = ltangpointx + ltangpointy - j;
		leftedg = (leftedg >= 0) ? leftedg : 0;
		s360_mset(octagonmap + j * w_src + leftedg, 1, (w_src >> 1) - leftedg * 2);
		s360_mset(octagonmap + j * w_src + (w_src >> 1) + leftedg, 1, (w_src >> 1) - leftedg * 2);
		s360_mset(octagonmap + (h_src - j - 1)* w_src + leftedg, 1, (w_src >> 1) - leftedg * 2);
		s360_mset(octagonmap + (h_src - j - 1)* w_src + (w_src >> 1) + leftedg, 1, (w_src >> 1) - leftedg * 2);
	}

	if (opt & S360_OPT_PAD)
	{
		if (cs == S360_COLORSPACE_YUV420)
		{
			pad_octagon_plane_circular((uint8 *)src, w_src, h_src, s_src, octagonmap);
		}
		else if (cs == S360_COLORSPACE_YUV420_10)
		{
			pad_octagon_plane_circular_10b((uint16 *)src, w_src, h_src, s_src, octagonmap);
		}
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

	for (j = 0; j<h_src; j++)
	{
		y = j;
		//phi = 3 * asin((double)(j + 0.5) / h_src - 0.5);
		//y = h_src * (phi + PI / 2) / PI - 0.5;
		int comy = (y >= h_src / 2) ? h_src - y - 1 : y;
		int octagonleft = ltangpointx + ltangpointy - comy;
		octagonleft = (octagonleft >= 0) ? octagonleft : 0;
		int octagonnum = w_src - octagonleft * 4;
		int c = 0;
		int cppleft = 0;
		while (map[j*w_src + cppleft] == 0 && cppleft < w_src) cppleft++;
		int cppnum = w_src - cppleft * 2;
		for (i = 0; i<w_src; i++)
		{
			if (map[j*w_src + i] == 1)
			{

				if (c >= (double)cppnum / 4 && c < (double)cppnum / 4 * 3)
					x = (c - (double)cppnum / 4) / cppnum * octagonnum + octagonleft;
				else if (c < (double)cppnum / 4)
					x = (double)c / cppnum * octagonnum + w_src / 4 * 3;
				else
					x = (c - (double)cppnum / 4 * 3) / cppnum * octagonnum + octagonleft + w_src / 2;

				c++;
				fn_resample(src, w_start, w_end, h_start, h_src, s_src, x, y, dst, i);
			}

		}
		dst = (void *)((uint8 *)dst + s_dst);
	}

	if (opt & S360_OPT_PAD)
	{
		if (cs == S360_COLORSPACE_YUV420)
		{
			pad_cpp_plane((uint8 *)dst0, w_src, h_src, s_src, map);
		}
		else if (cs == S360_COLORSPACE_YUV420_10)
		{
			pad_cpp_plane_10b((uint16 *)dst0, w_src, h_src, s_src, map);
		}
	}

	s360_mfree(map2CPP);
	s360_mfree(map);
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
	int w_dst, int h_dst, void * dst, int s_dst, int cs)
{
	//CPP To 2Octagon
	resample_fn fn_resample;
	uint8 * map2CPP; //2CPPmap
	uint8 * octagonmap;//??????map
	uint8 * map;//CPPmap
	int     i, j;
	double  phi;
	double  x, y;

	fn_resample = resample_fp(cs);

	map2CPP = (uint8 *)s360_malloc(sizeof(uint8) * w_src * h_src);
	map = (uint8 *)s360_malloc(sizeof(uint8) * w_src * h_src);
	cpp_map_plane(w_src, h_src, s_src, map2CPP);
	CPP_map_plane(w_src, h_src, s_src, map);
	octagonmap = (uint8 *)s360_malloc(sizeof(uint8) * w_src * h_src);
	s360_mset(octagonmap, 0, w_src * h_src);

	//????????????map
	int lcons45lean = 0, lcons45leantemp = 0, rcons45lean = 0, rcons45leantemp = 0;
	int lastleftedg = 0, lastrightedg = 0;
	int ltangpointy = 0, rtangpointy = 0, ltangpointx = 0, rtangpointx = 0;
	int leftedg, rightedg;
	for (j = 0; j < (h_src >> 1); j++)
	{
		for (i = 0; i < w_src; i++)
		{
			if (map2CPP[i + j*w_src] == 1)
			{
				leftedg = i;
				break;
			}
		}

		if (leftedg == lastleftedg - 1)
			lcons45leantemp++;
		else
		{
			if (lcons45lean < lcons45leantemp)
			{
				lcons45lean = lcons45leantemp;
				ltangpointy = j - 1;
				ltangpointx = lastleftedg;
			}
			lcons45leantemp = 0;
		}
		lastleftedg = leftedg;
	}

	for (j = 0; j < (h_src >> 1); j++)
	{
		leftedg = ltangpointx + ltangpointy - j;
		leftedg = (leftedg >= 0) ? leftedg : 0;
		s360_mset(octagonmap + j * w_src + leftedg, 1, (w_src >> 1) - leftedg * 2);
		s360_mset(octagonmap + j * w_src + (w_src >> 1) + leftedg, 1, (w_src >> 1) - leftedg * 2);
		s360_mset(octagonmap + (h_src - j - 1)* w_src + leftedg, 1, (w_src >> 1) - leftedg * 2);
		s360_mset(octagonmap + (h_src - j - 1)* w_src + (w_src >> 1) + leftedg, 1, (w_src >> 1) - leftedg * 2);
	}

	if (cs == S360_COLORSPACE_YUV420)
	{
		//pad_cpp_plane((uint8 *)(src), w_src, h_src, s_src, map);
		pad_cpp_plane_circular((uint8 *)(src), w_src, h_src, s_src, map);
	}
	else if (cs == S360_COLORSPACE_YUV420_10)
	{
		//pad_cpp_plane_10b((uint16 *)(src), w_src, h_src, s_src, map);
		pad_cpp_plane_circular_10b((uint16 *)(src), w_src, h_src, s_src, map);
		s_dst <<= 1;
	}

	for (j = 0; j<h_dst; j++)
	{
		y = j;
		//y = h_src * (.5 + sin(PI / 3 * ((double)(j + 0.5) / h_dst - .5f))) - 0.5;
		int comj = (j >= h_src / 2) ? h_src - j - 1 : j;
		int octagonleft = ltangpointx + ltangpointy - comj;
		octagonleft = (octagonleft >= 0) ? octagonleft : 0;
		int octagonnum = w_src - octagonleft * 4;
		int c = 0;
		int cppleft = 0;
		while (map[j*w_src + cppleft] == 0 && cppleft < w_src) cppleft++;
		int cppnum = w_src - cppleft * 2;
		for (i = 0; i<w_dst; i++)
		{
			if (octagonmap[j*w_src + i] == 1)
			{
				if (c < (double)octagonnum / 2)
					x = (double)c / octagonnum * cppnum + cppleft + (double)cppnum / 4;
				else if (c >= (double)octagonnum / 4 * 3)
					x = (c - (double)octagonnum / 4 * 3) / octagonnum * cppnum + cppleft;
				else
					x = (c - (double)octagonnum / 2) / octagonnum * cppnum + cppleft + (double)cppnum / 4 * 3;

				c++;
				fn_resample(src, 0, w_src, 0, h_src, s_src, x, y, dst, i);
			}
		}
		dst = (void *)((uint8 *)dst + s_dst);
	}

	s360_mfree(map);
	s360_mfree(map2CPP);
	s360_mfree(octagonmap);
}

int s360_cpp_to_erp(S360_IMAGE * img_src, S360_IMAGE * img_dst, int opt, S360_MAP * map)
{
	int i, w_src, w_dst, h_src, h_dst;
	s360_img_reset(img_dst);
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
				img_src->buffer[i], w_dst, h_dst, img_dst->buffer[i], \
				img_dst->stride[i], img_src->colorspace);
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
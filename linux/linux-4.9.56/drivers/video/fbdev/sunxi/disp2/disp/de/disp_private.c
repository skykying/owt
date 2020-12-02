/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "disp_private.h"
#include "../dev_disp.h"

extern struct disp_drv_info g_disp_drv;

#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)

s32 disp_delay_ms(u32 ms)
{
#if defined(__LINUX_PLAT__)
	mdelay(ms);
#endif
#ifdef __UBOOT_PLAT__
	__msdelay(ms);
#endif
	return 0;
}

#else

s32 disp_delay_ms(u32 ms)
{
#if defined(__LINUX_PLAT__)
	u32 timeout = msecs_to_jiffies(ms);

	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(timeout);
#endif
#ifdef __UBOOT_PLAT__
	__msdelay(ms);
#endif
	return 0;
}

#endif				/*endif SUPPORT_EINK */

s32 disp_delay_us(u32 us)
{
	udelay(us);

	return 0;
}

u32 dump_layer_config(struct disp_layer_config_data *data)
{
	u32 count = 0;
	char buf[512];

	count +=
	    sprintf(buf + count, " %6s ",
		    (data->config.info.mode == LAYER_MODE_BUFFER) ?
		    "buffer" : "color");
	count +=
	    sprintf(buf + count, " %8s ",
		    (data->config.enable == 1) ? "enable" : "disable");
	count += sprintf(buf + count, "ch[%1d] ", data->config.channel);
	count += sprintf(buf + count, "lyr[%1d] ", data->config.layer_id);
	count += sprintf(buf + count, "z[%1d] ", data->config.info.zorder);
	count +=
	    sprintf(buf + count, "pre_m[%1s] ",
		    (data->config.info.fb.pre_multiply) ? "Y" : "N");
	count +=
	    sprintf(buf + count, "alpha[%5s %3d] ",
		    (data->config.info.alpha_mode) ? "globl" : "pixel",
		    data->config.info.alpha_value);
	count += sprintf(buf + count, "fmt[%3d] ", data->config.info.fb.format);
	count +=
	    sprintf(buf + count, "size[%4d,%4d;%4d,%4d;%4d,%4d] ",
		    data->config.info.fb.size[0].width,
		    data->config.info.fb.size[0].height,
		    data->config.info.fb.size[0].width,
		    data->config.info.fb.size[0].height,
		    data->config.info.fb.size[0].width,
		    data->config.info.fb.size[0].height);
	count +=
	    sprintf(buf + count, "crop[%4d,%4d,%4d,%4d] ",
		    (u32) (data->config.info.fb.crop.x >> 32),
		    (u32) (data->config.info.fb.crop.y >> 32),
		    (u32) (data->config.info.fb.crop.width >> 32),
		    (u32) (data->config.info.fb.crop.height >> 32));
	count +=
	    sprintf(buf + count, "frame[%4d,%4d,%4d,%4d] ",
		    data->config.info.screen_win.x,
		    data->config.info.screen_win.y,
		    data->config.info.screen_win.width,
		    data->config.info.screen_win.height);
	count +=
	    sprintf(buf + count, "addr[%8llx,%8llx,%8llx] ",
		    data->config.info.fb.addr[0], data->config.info.fb.addr[1],
		    data->config.info.fb.addr[2]);
	count += sprintf(buf + count, "flag[0x%8x] ", data->flag);
	count += sprintf(buf + count, "\n");

	DE_WRN("%s", buf);
	return count;
}

void *disp_vmap(unsigned long phys_addr, unsigned long size)
{
	int npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	struct page *cur_page = phys_to_page(phys_addr);
	pgprot_t pgprot;
	void *vaddr = NULL;
	int i;

	if (!pages)
		return NULL;

	for (i = 0; i < npages; i++)
		*(tmp++) = cur_page++;

	pgprot = PAGE_KERNEL;
	vaddr = vmap(pages, npages, VM_MAP, pgprot);

	vfree(pages);
	return vaddr;
}

void disp_vunmap(const void *vaddr)
{
	vunmap(vaddr);
}

static int disp_dma_map_core(int fd, struct dmabuf_item *item)
{
#if defined(__LINUX_PLAT__)
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt, *sgt_bak;
	struct scatterlist *sgl, *sgl_bak;
	s32 sg_count = 0;
	int ret = -1;
	int i;

	if (fd < 0) {
		DE_WRN("dma_buf_id(%d) is invalid\n", fd);
		goto exit;
	}
	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		DE_WRN("dma_buf_get failed, fd=%d\n", fd);
		goto exit;
	}
	attachment = dma_buf_attach(dmabuf, g_disp_drv.dev);
	if (IS_ERR(attachment)) {
		DE_WRN("dma_buf_attach failed\n");
		goto err_buf_put;
	}
	sgt = dma_buf_map_attachment(attachment, DMA_FROM_DEVICE);
	if (IS_ERR_OR_NULL(sgt)) {
		DE_WRN("dma_buf_map_attachment failed\n");
		goto err_buf_detach;
	}

	sgt_bak = kmalloc(sizeof(struct sg_table), GFP_KERNEL | __GFP_ZERO);
	if (sgt_bak == NULL) {
		DE_WRN("alloc sgt fail\n");
		goto err_buf_unmap;
	}
	ret = sg_alloc_table(sgt_bak, sgt->nents, GFP_KERNEL);
	if (ret != 0) {
		DE_WRN("alloc sgt fail\n");
		goto err_kfree;
	}
	sgl_bak = sgt_bak->sgl;
	for_each_sg(sgt->sgl, sgl, sgt->nents, i)  {
		sg_set_page(sgl_bak, sg_page(sgl), sgl->length, sgl->offset);
		sgl_bak = sg_next(sgl_bak);
	}
#if defined(SUPPORT_EINK)
	sg_count = dma_map_sg(g_disp_drv.dev, sgt_bak->sgl,
			      sgt_bak->nents, DMA_FROM_DEVICE);
#else
	sg_count =
	    dma_map_sg_attrs(g_disp_drv.dev, sgt_bak->sgl, sgt_bak->nents,
			     DMA_FROM_DEVICE, DMA_ATTR_SKIP_CPU_SYNC);
#endif

	if (sg_count != 1) {
		DE_WRN("dma_map_sg failed:%d\n", sg_count);
		ret = -1;
		goto err_sgt_free;
	}

	item->fd = fd;
	item->buf = dmabuf;
	item->sgt = sgt_bak;
	item->attachment = attachment;
	item->dma_addr = sg_dma_address(sgt_bak->sgl);
	ret = 0;

	goto exit;

err_sgt_free:
	sg_free_table(sgt_bak);
err_kfree:
	kfree(sgt_bak);
err_buf_unmap:
	/* unmap attachment sgt, not sgt_bak, cause it's not alloc yet! */
	dma_buf_unmap_attachment(attachment, sgt, DMA_FROM_DEVICE);
err_buf_detach:
	dma_buf_detach(dmabuf, attachment);
err_buf_put:
	dma_buf_put(dmabuf);
exit:
	return ret;
#endif
	return 0;
}

static void disp_dma_unmap_core(struct dmabuf_item *item)
{
#if defined(__LINUX_PLAT__)

#if defined(SUPPORT_EINK)
	dma_unmap_sg(g_disp_drv.dev, item->sgt->sgl,
			      item->sgt->nents, DMA_FROM_DEVICE);
#else

	dma_unmap_sg_attrs(g_disp_drv.dev, item->sgt->sgl,
			      item->sgt->nents, DMA_FROM_DEVICE,
			      DMA_ATTR_SKIP_CPU_SYNC);
#endif
	dma_buf_unmap_attachment(item->attachment, item->sgt, DMA_FROM_DEVICE);
	sg_free_table(item->sgt);
	kfree(item->sgt);
	dma_buf_detach(item->buf, item->attachment);
	dma_buf_put(item->buf);
#endif
}

struct dmabuf_item *disp_dma_map(int fd)
{
#if defined(__LINUX_PLAT__)
	struct dmabuf_item *item = NULL;

	item = kmalloc(sizeof(struct dmabuf_item),
			      GFP_KERNEL | __GFP_ZERO);

	if (item == NULL) {
		DE_WRN("malloc memory of size %d fail!\n",
		       (unsigned int)sizeof(struct dmabuf_item));
		goto exit;
	}
	if (disp_dma_map_core(fd, item) != 0) {
		kfree(item);
		item = NULL;
	}

exit:
	return item;
#else

	return NULL;
#endif
}

void disp_dma_unmap(struct dmabuf_item *item)
{
#if defined(__LINUX_PLAT__)
	disp_dma_unmap_core(item);
	kfree(item);
#endif
}

static struct disp_format_attr fmt_attr_tbl[] = {
	/* format                            bits */
	/* hor_rsample(u,v) */
	/* ver_rsample(u,v) */
	/* uvc */
	/* interleave */
	/* factor */
	/* div */

	{ DISP_FORMAT_ARGB_8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_ABGR_8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_RGBA_8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_BGRA_8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_XRGB_8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_XBGR_8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_RGBX_8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_BGRX_8888, 8,  1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_RGB_888, 8,  1, 1, 1, 1, 0, 1, 3, 1},
	{ DISP_FORMAT_BGR_888, 8,  1, 1, 1, 1, 0, 1, 3, 1},
	{ DISP_FORMAT_RGB_565, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_BGR_565, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_ARGB_4444, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_ABGR_4444, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_RGBA_4444, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_BGRA_4444, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_ARGB_1555, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_ABGR_1555, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_RGBA_5551, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_BGRA_5551, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_A2R10G10B10, 10, 1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_A2B10G10R10, 10, 1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_R10G10B10A2, 10, 1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_B10G10R10A2, 10, 1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_YUV444_I_AYUV, 8,  1, 1, 1, 1, 0, 1, 3, 1},
	{ DISP_FORMAT_YUV444_I_VUYA, 8,  1, 1, 1, 1, 0, 1, 3, 1},
	{ DISP_FORMAT_YUV422_I_YVYU, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_YUV422_I_YUYV, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_YUV422_I_UYVY, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_YUV422_I_VYUY, 8,  1, 1, 1, 1, 0, 1, 2, 1},
	{ DISP_FORMAT_YUV444_P, 8,  1, 1, 1, 1, 0, 1, 1, 1},
	{ DISP_FORMAT_YUV422_P, 8,  2, 2, 1, 1, 0, 0, 2, 1},
	{ DISP_FORMAT_YUV420_P, 8,  2, 2, 2, 2, 0, 0, 3, 2},
	{ DISP_FORMAT_YUV411_P, 8,  4, 4, 1, 1, 0, 0, 3, 2},
	{ DISP_FORMAT_YUV422_SP_UVUV, 8,  2, 2, 1, 1, 1, 0, 2, 1},
	{ DISP_FORMAT_YUV422_SP_VUVU, 8,  2, 2, 1, 1, 1, 0, 2, 1},
	{ DISP_FORMAT_YUV420_SP_UVUV, 8,  2, 2, 2, 2, 1, 0, 3, 2},
	{ DISP_FORMAT_YUV420_SP_VUVU, 8,  2, 2, 2, 2, 1, 0, 3, 2},
	{ DISP_FORMAT_YUV411_SP_UVUV, 8,  4, 4, 1, 1, 1, 0, 3, 2},
	{ DISP_FORMAT_YUV411_SP_VUVU, 8,  4, 4, 1, 1, 1, 0, 3, 2},
	{ DISP_FORMAT_8BIT_GRAY, 8,  1, 1, 1, 1, 0, 0, 1, 1},
	{ DISP_FORMAT_YUV444_I_AYUV_10BIT, 10, 1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_YUV444_I_VUYA_10BIT, 10, 1, 1, 1, 1, 0, 1, 4, 1},
	{ DISP_FORMAT_YUV422_I_YVYU_10BIT, 10, 1, 1, 1, 1, 0, 1, 3, 1},
	{ DISP_FORMAT_YUV422_I_YUYV_10BIT, 10, 1, 1, 1, 1, 0, 1, 3, 1},
	{ DISP_FORMAT_YUV422_I_UYVY_10BIT, 10, 1, 1, 1, 1, 0, 1, 3, 1},
	{ DISP_FORMAT_YUV422_I_VYUY_10BIT, 10, 1, 1, 1, 1, 0, 1, 3, 1},
	{ DISP_FORMAT_YUV444_P_10BIT, 10, 1, 1, 1, 1, 0, 0, 6, 1},
	{ DISP_FORMAT_YUV422_P_10BIT, 10, 2, 2, 1, 1, 0, 0, 4, 1},
	{ DISP_FORMAT_YUV420_P_10BIT, 10, 2, 2, 2, 2, 0, 0, 3, 1},
	{ DISP_FORMAT_YUV411_P_10BIT, 10, 4, 4, 1, 1, 0, 0, 3, 1},
	{ DISP_FORMAT_YUV422_SP_UVUV_10BIT, 10, 2, 2, 1, 1, 1, 0, 4, 1},
	{ DISP_FORMAT_YUV422_SP_VUVU_10BIT, 10, 2, 2, 1, 1, 1, 0, 4, 1},
	{ DISP_FORMAT_YUV420_SP_UVUV_10BIT, 10, 2, 2, 2, 2, 1, 0, 3, 1},
	{ DISP_FORMAT_YUV420_SP_VUVU_10BIT, 10, 2, 2, 2, 2, 1, 0, 3, 1},
	{ DISP_FORMAT_YUV411_SP_UVUV_10BIT, 10, 4, 4, 1, 1, 1, 0, 3, 1},
	{ DISP_FORMAT_YUV411_SP_VUVU_10BIT, 10, 4, 4, 1, 1, 1, 0, 3, 1},
};

/* @left_eye: indicate left eye buffer when true */
s32 disp_set_fb_info(struct fb_address_transfer *fb, bool left_eye)
{
	s32 ret = -1;
	u32 i = 0;
	u32 len = ARRAY_SIZE(fmt_attr_tbl);
	u32 y_width, y_height, u_width, u_height;
	u32 y_pitch, u_pitch;
	u32 y_size, u_size;
	unsigned long long fb_addr[3];


	y_width = fb->size[0].width;
	y_height = fb->size[0].height;
	u_width = fb->size[1].width;
	u_height = fb->size[1].height;
	u_width = (u_width == 0) ? y_width : u_width;
	u_height = (u_height == 0) ? y_height : u_height;

	fb_addr[0] = fb->dma_addr;

	if (fb->format >= DISP_FORMAT_MAX) {
		DE_WRN("%s, format 0x%x is out of range\n", __func__,
			fb->format);
		goto exit;
	}

	for (i = 0; i < len; ++i) {

		if (fmt_attr_tbl[i].format == fb->format) {
			y_pitch = y_width *
			    ((fmt_attr_tbl[i].bits == 8) ? 1 : 2);
			u_pitch = u_width *
			    ((fmt_attr_tbl[i].bits == 8) ? 1 : 2) *
			    (fmt_attr_tbl[i].uvc + 1);

			y_pitch = DISPALIGN(y_pitch, fb->align[0]);
			u_pitch = DISPALIGN(u_pitch, fb->align[1]);
			y_size = y_pitch * y_height;
			u_size = u_pitch * u_height;

			fb_addr[1] = fb->dma_addr + y_size; /* u */
			fb_addr[2] = fb->dma_addr + y_size + u_size; /* v */

			if (fb->format == DISP_FORMAT_YUV420_P ||
				fb->format == DISP_FORMAT_YUV420_P_10BIT) {
				/* v */
				fb_addr[1] = fb->dma_addr + y_size + u_size;
				fb_addr[2] = fb->dma_addr + y_size; /* u */
			}

			ret = 0;
			break;
		}
	}
	if (ret != 0) {
		DE_WRN("%s, format 0x%x is invalid\n", __func__,
			fb->format);
	} else {
		if (left_eye) {
			fb->addr[0] = fb_addr[0];
			fb->addr[1] = fb_addr[1];
			fb->addr[2] = fb_addr[2];
			fb->trd_right_addr[0] = fb_addr[0];
			fb->trd_right_addr[1] = fb_addr[1];
			fb->trd_right_addr[2] = fb_addr[2];
		} else {
			fb->trd_right_addr[0] = fb_addr[0];
			fb->trd_right_addr[1] = fb_addr[1];
			fb->trd_right_addr[2] = fb_addr[2];
		}
	}
exit:
	return ret;
}

s32 disp_set_fb_base_on_depth(struct fb_address_transfer *fb)
{
	s32 ret = -1;
	u32 i = 0;
	u32 len = ARRAY_SIZE(fmt_attr_tbl);
	int depth = fb->depth;
	unsigned long long abs_depth = (depth > 0) ?
				  depth : (-depth);
	/*
	 * 1: left & right move closer, right buffer address move right
	 * 0: in opposite direction
	 */
	unsigned int direction = (depth > 0) ? 1 : 0;
	unsigned int offset = 0;

	if (fb->format >= DISP_FORMAT_MAX) {
		DE_WRN("%s, format 0x%x is out of range\n", __func__,
			fb->format);
		goto exit;
	}

	for (i = 0; i < len; ++i) {
		if (fmt_attr_tbl[i].format == fb->format) {
			offset = fmt_attr_tbl[i].factor / fmt_attr_tbl[i].div;
			offset = offset * (unsigned int)abs_depth;

			ret = 0;
			break;
		}
	}
	if (ret != 0) {
		DE_WRN("%s, format 0x%x is invalid\n", __func__,
			fb->format);
	} else {
		if (direction == 0) {
			fb->addr[0] += offset;
			fb->addr[1] += offset;
			fb->addr[2] += offset;
		} else {
			fb->trd_right_addr[0] += offset;
			fb->trd_right_addr[1] += offset;
			fb->trd_right_addr[2] += offset;
		}
	}
exit:
	return ret;
}

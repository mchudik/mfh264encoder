/*
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2017 Martin <<user@hostname.org>>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-mfh264encoder
 *
 * FIXME:Describe mfh264encoder here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! mfh264encoder ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/controller/controller.h>

#include "gstmfh264encoder.h"

#pragma comment(lib, "gstvideo-1.0.lib")

GST_DEBUG_CATEGORY_STATIC (gst_mfh264encoder_debug);
#define GST_CAT_DEFAULT gst_mfh264encoder_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
};

/* the capabilities of the inputs and outputs.
 *
 * FIXME:describe the real formats here.
 */
static GstStaticPadTemplate sink_template =
GST_STATIC_PAD_TEMPLATE (
  "sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("ANY")
);

static GstStaticPadTemplate src_template =
GST_STATIC_PAD_TEMPLATE (
  "src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("ANY")
);

#define gst_mfh264encoder_parent_class parent_class
G_DEFINE_TYPE (Gstmfh264encoder, gst_mfh264encoder, GST_TYPE_VIDEO_ENCODER);

static void gst_mfh264encoder_set_property (GObject * object, guint prop_id,
	const GValue * value, GParamSpec * pspec);
static void gst_mfh264encoder_get_property (GObject * object, guint prop_id,
	GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_mfh264encoder_handle_frame(GstVideoEncoder * encoder, 
	GstVideoCodecFrame * frame);

static gboolean gst_mfh264encoder_set_format(GstVideoEncoder * encoder,
	GstVideoCodecState * state);

static GstCaps * gst_mfh264encoderc_getcaps(GstVideoEncoder * encoder,
	GstCaps * filter);

/* GObject vmethod implementations */

/* initialize the mfh264encoder's class */
static void
gst_mfh264encoder_class_init (Gstmfh264encoderClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_mfh264encoder_set_property;
  gobject_class->get_property = gst_mfh264encoder_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
	g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
		  FALSE, (GParamFlags)(G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE)));

  gst_element_class_set_details_simple (gstelement_class,
	"mfh264encoder",
	"Generic/Filter",
	"FIXME:Generic Template Filter",
	"Martin <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
	  gst_static_pad_template_get (&src_template));
  gst_element_class_add_pad_template (gstelement_class,
	  gst_static_pad_template_get (&sink_template));

  GST_VIDEO_ENCODER_CLASS(klass)->handle_frame =
	  GST_DEBUG_FUNCPTR(gst_mfh264encoder_handle_frame);

  GST_VIDEO_ENCODER_CLASS(klass)->set_format =
	  GST_DEBUG_FUNCPTR(gst_mfh264encoder_set_format);
 
  GST_VIDEO_ENCODER_CLASS(klass)->getcaps =
	  GST_DEBUG_FUNCPTR(gst_mfh264encoderc_getcaps);

  /* debug category for fltering log messages
   *
   * FIXME:exchange the string 'Template mfh264encoder' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_mfh264encoder_debug, "mfh264encoder", 0, "Template mfh264encoder");
}

/* initialize the new element
 * initialize instance structure
 */
static void
gst_mfh264encoder_init (Gstmfh264encoder *filter)
{
	HRESULT hr = S_OK;
	filter->silent = FALSE;
}

static void
gst_mfh264encoder_set_property (GObject * object, guint prop_id,
	const GValue * value, GParamSpec * pspec)
{
  Gstmfh264encoder *filter = GST_MFH264ENCODER (object);

  switch (prop_id) {
	case PROP_SILENT:
	  filter->silent = g_value_get_boolean (value);
	  break;
	default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	  break;
  }
}

static void
gst_mfh264encoder_get_property (GObject * object, guint prop_id,
	GValue * value, GParamSpec * pspec)
{
  Gstmfh264encoder *filter = GST_MFH264ENCODER (object);

  switch (prop_id) {
	case PROP_SILENT:
	  g_value_set_boolean (value, filter->silent);
	  break;
	default:
	  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	  break;
  }
}

/* GstVideoEncoder vmethod implementations */

/* this function does the actual processing
 */
static GstFlowReturn
gst_mfh264encoder_handle_frame(GstVideoEncoder * encoder, GstVideoCodecFrame * frame)
{
	Gstmfh264encoder *filter = GST_MFH264ENCODER(encoder);
	GstMapInfo mapIn, mapOut;

	// Encoding is happening here
	if (filter->pH264Encoder) {
		gst_buffer_map(frame->input_buffer, &mapIn, GST_MAP_READ);
		frame->output_buffer = gst_buffer_new_and_alloc(mapIn.size);
		gst_buffer_map(frame->output_buffer, &mapOut, GST_MAP_WRITE);
		if (filter->pH264Encoder->Encode(
			(BYTE *)mapIn.data,
			mapIn.size,
			(BYTE *)mapOut.data,
			(UINT32*)&mapOut.size,
			GST_BUFFER_TIMESTAMP(frame->input_buffer),
			GST_BUFFER_DURATION(frame->input_buffer)
		) != S_OK) {
		;// Process errors here
		}
		GST_BUFFER_TIMESTAMP(frame->output_buffer);
		GST_BUFFER_DURATION(frame->output_buffer);

		gst_buffer_unmap(frame->input_buffer, &mapIn);
		gst_buffer_unmap(frame->output_buffer, &mapOut);
	}

	return gst_video_encoder_finish_frame(encoder, frame);
}

static gboolean
gst_mfh264encoder_set_format(GstVideoEncoder * encoder,	GstVideoCodecState * state)
{
	Gstmfh264encoder *filter = GST_MFH264ENCODER(encoder);
	GstVideoInfo *info = &state->info;
	
	// Initialize Media Foundation h.264 encoder
	if (filter->pH264Encoder == NULL) {
		filter->pH264Encoder = new (std::nothrow) CEncoderH264("C:\\Echo360\\Temp\\Aoutput.mp4", FALSE, TRUE);
		if (filter->pH264Encoder) {
			if (filter->pH264Encoder->ConfigureEncoder(
				gst_video_format_to_fourcc(info->finfo->format),	// FCC('YUY2')
				GST_VIDEO_INFO_WIDTH(info),					// Frame width
				GST_VIDEO_INFO_HEIGHT(info),				// Frame Height
				{ (DWORD)info->fps_n, (DWORD)info->fps_d },	// Frame Rate
				90,			// VBR Quality
				1000000,	// MeanBitrate
				2000000		// MaxBitrate
			) == S_OK) {
				if (filter->silent == FALSE)
					g_print("\n-------------------SUCCESS Initializing Encoder!!!------------------\n");
			}
		}
	}

	// Define compressed output format here
	GstCaps *caps = gst_caps_new_simple("video/x-h264",
		"format", G_TYPE_STRING, "h264",
		"width", G_TYPE_INT, GST_VIDEO_INFO_WIDTH(info),
		"height", G_TYPE_INT, GST_VIDEO_INFO_HEIGHT(info),
		"framerate", GST_TYPE_FRACTION, info->fps_n, info->fps_d,
		"pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
		"interlace-mode", G_TYPE_STRING, gst_video_interlace_mode_to_string(GST_VIDEO_INTERLACE_MODE_PROGRESSIVE),
		NULL);

	GstVideoCodecState *output_format = gst_video_encoder_set_output_state(encoder, caps, state);
	gst_video_codec_state_unref(output_format);

	return TRUE;
}

static GstCaps *
gst_mfh264encoderc_getcaps(GstVideoEncoder * encoder, GstCaps * filter)
{
	// We need to give format option here. Supported formats are: 
	//   MFVideoFormat_I420
	//   MFVideoFormat_IYUV
	//   MFVideoFormat_NV12
	//   MFVideoFormat_YUY2
	//   MFVideoFormat_YV12
	GstCaps *caps = gst_caps_new_simple("video/x-raw",
		"format", G_TYPE_STRING, "YUY2",
		"pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
		"interlace-mode", G_TYPE_STRING, gst_video_interlace_mode_to_string(GST_VIDEO_INTERLACE_MODE_PROGRESSIVE),
		NULL);
	return caps;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
mfh264encoder_init (GstPlugin * mfh264encoder)
{
  return gst_element_register (mfh264encoder, "mfh264encoder", GST_RANK_NONE,
	  GST_TYPE_MFH264ENCODER);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
* in configure.ac and then written into and defined in config.h, but we can
* just set it ourselves here in case someone doesn't use autotools to
* compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
*/
#ifndef PACKAGE
#define PACKAGE "mfh264encoder"
#endif
#ifndef VERSION
#define VERSION "1.0"
#endif

/* gstreamer looks for this structure to register mfh264encoders
 *
 * FIXME:exchange the string 'Template mfh264encoder' with you mfh264encoder description
 */
GST_PLUGIN_DEFINE (
	GST_VERSION_MAJOR,
	GST_VERSION_MINOR,
	mfh264encoder,
	"Template mfh264encoder",
	mfh264encoder_init,
	VERSION,
	"LGPL",
	"GStreamer",
	"http://gstreamer.net/"
)

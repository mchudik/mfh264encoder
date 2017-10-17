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
#include <gst/base/base.h>
#include <gst/controller/controller.h>

#include "gstmfh264encoder.h"
#pragma comment(lib, "gstbase-1.0.lib")

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
G_DEFINE_TYPE (Gstmfh264encoder, gst_mfh264encoder, GST_TYPE_BASE_TRANSFORM);

static void gst_mfh264encoder_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_mfh264encoder_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_mfh264encoder_transform_ip (GstBaseTransform * base,
    GstBuffer * outbuf);

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
          FALSE, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  gst_element_class_set_details_simple (gstelement_class,
    "mfh264encoder",
    "Generic/Filter",
    "FIXME:Generic Template Filter",
    "Martin <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_template));

  GST_BASE_TRANSFORM_CLASS (klass)->transform_ip =
      GST_DEBUG_FUNCPTR (gst_mfh264encoder_transform_ip);

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

/* GstBaseTransform vmethod implementations */

/* this function does the actual processing
 */
static GstFlowReturn
gst_mfh264encoder_transform_ip (GstBaseTransform * base, GstBuffer * outbuf)
{
  Gstmfh264encoder *filter = GST_MFH264ENCODER (base);

  if (GST_CLOCK_TIME_IS_VALID (GST_BUFFER_TIMESTAMP (outbuf)))
    gst_object_sync_values (GST_OBJECT (filter), GST_BUFFER_TIMESTAMP (outbuf));

  if (filter->silent == FALSE)
    g_print ("I'm plugged, therefore I'm in.\n");
  
  /* FIXME: do something interesting here.  This simply copies the source
   * to the destination. */

  return GST_FLOW_OK;
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

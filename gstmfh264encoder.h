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
 
#ifndef __GST_MFH264ENCODER_H__
#define __GST_MFH264ENCODER_H__

#include <gst/gst.h>
#include <gst/video/gstvideoencoder.h>

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include "MFEncoderH264.h"

#pragma comment(lib, "gstreamer-1.0.lib")
#pragma comment(lib, "gobject-2.0.lib")
#pragma comment(lib, "glib-2.0.lib")
#pragma comment(lib, "intl.lib")

G_BEGIN_DECLS

#define GST_TYPE_MFH264ENCODER \
  (gst_mfh264encoder_get_type())
#define GST_MFH264ENCODER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MFH264ENCODER,Gstmfh264encoder))
#define GST_MFH264ENCODER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MFH264ENCODER,Gstmfh264encoderClass))
#define GST_IS_MFH264ENCODER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MFH264ENCODER))
#define GST_IS_MFH264ENCODER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MFH264ENCODER))

typedef struct _Gstmfh264encoder      Gstmfh264encoder;
typedef struct _Gstmfh264encoderClass Gstmfh264encoderClass;

struct _Gstmfh264encoder {
  GstVideoEncoder element;

  gboolean silent;
  CEncoderH264*	pH264Encoder;
};

struct _Gstmfh264encoderClass {
  GstVideoEncoderClass parent_class;
};

GType gst_mfh264encoder_get_type (void);

G_END_DECLS

#endif /* __GST_MFH264ENCODER_H__ */

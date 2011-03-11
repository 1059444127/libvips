
/* Generated data (by glib-mkenums) */

/* auto-generated enums for vips introspection */

#include <vips/vips.h>
/* enumerations from "../../libvips/include/vips/util.h" */
GType
vips_token_get_type( void )
{
	static GType etype = 0;

	if( etype == 0 ) {
		static const GEnumValue values[] = {
			{VIPS_TOKEN_LEFT, "VIPS_TOKEN_LEFT", "left"},
			{VIPS_TOKEN_RIGHT, "VIPS_TOKEN_RIGHT", "right"},
			{VIPS_TOKEN_STRING, "VIPS_TOKEN_STRING", "string"},
			{VIPS_TOKEN_EQUALS, "VIPS_TOKEN_EQUALS", "equals"},
			{VIPS_TOKEN_COMMA, "VIPS_TOKEN_COMMA", "comma"},
			{0, NULL, NULL}
		};
		
		etype = g_enum_register_static( "VipsToken", values );
	}

	return( etype );
}
/* enumerations from "../../libvips/include/vips/image.h" */
GType
vips_demand_style_get_type( void )
{
	static GType etype = 0;

	if( etype == 0 ) {
		static const GEnumValue values[] = {
			{VIPS_DEMAND_STYLE_SMALLTILE, "VIPS_DEMAND_STYLE_SMALLTILE", "smalltile"},
			{VIPS_DEMAND_STYLE_FATSTRIP, "VIPS_DEMAND_STYLE_FATSTRIP", "fatstrip"},
			{VIPS_DEMAND_STYLE_THINSTRIP, "VIPS_DEMAND_STYLE_THINSTRIP", "thinstrip"},
			{VIPS_DEMAND_STYLE_ANY, "VIPS_DEMAND_STYLE_ANY", "any"},
			{0, NULL, NULL}
		};
		
		etype = g_enum_register_static( "VipsDemandStyle", values );
	}

	return( etype );
}
GType
vips_image_type_get_type( void )
{
	static GType etype = 0;

	if( etype == 0 ) {
		static const GEnumValue values[] = {
			{VIPS_IMAGE_NONE, "VIPS_IMAGE_NONE", "none"},
			{VIPS_IMAGE_SETBUF, "VIPS_IMAGE_SETBUF", "setbuf"},
			{VIPS_IMAGE_SETBUF_FOREIGN, "VIPS_IMAGE_SETBUF_FOREIGN", "setbuf-foreign"},
			{VIPS_IMAGE_OPENIN, "VIPS_IMAGE_OPENIN", "openin"},
			{VIPS_IMAGE_MMAPIN, "VIPS_IMAGE_MMAPIN", "mmapin"},
			{VIPS_IMAGE_MMAPINRW, "VIPS_IMAGE_MMAPINRW", "mmapinrw"},
			{VIPS_IMAGE_OPENOUT, "VIPS_IMAGE_OPENOUT", "openout"},
			{VIPS_IMAGE_PARTIAL, "VIPS_IMAGE_PARTIAL", "partial"},
			{0, NULL, NULL}
		};
		
		etype = g_enum_register_static( "VipsImageType", values );
	}

	return( etype );
}
GType
vips_interpretation_get_type( void )
{
	static GType etype = 0;

	if( etype == 0 ) {
		static const GEnumValue values[] = {
			{VIPS_INTERPRETATION_MULTIBAND, "VIPS_INTERPRETATION_MULTIBAND", "multiband"},
			{VIPS_INTERPRETATION_B_W, "VIPS_INTERPRETATION_B_W", "b-w"},
			{VIPS_INTERPRETATION_HISTOGRAM, "VIPS_INTERPRETATION_HISTOGRAM", "histogram"},
			{VIPS_INTERPRETATION_FOURIER, "VIPS_INTERPRETATION_FOURIER", "fourier"},
			{VIPS_INTERPRETATION_XYZ, "VIPS_INTERPRETATION_XYZ", "xyz"},
			{VIPS_INTERPRETATION_LAB, "VIPS_INTERPRETATION_LAB", "lab"},
			{VIPS_INTERPRETATION_CMYK, "VIPS_INTERPRETATION_CMYK", "cmyk"},
			{VIPS_INTERPRETATION_LABQ, "VIPS_INTERPRETATION_LABQ", "labq"},
			{VIPS_INTERPRETATION_RGB, "VIPS_INTERPRETATION_RGB", "rgb"},
			{VIPS_INTERPRETATION_UCS, "VIPS_INTERPRETATION_UCS", "ucs"},
			{VIPS_INTERPRETATION_LCH, "VIPS_INTERPRETATION_LCH", "lch"},
			{VIPS_INTERPRETATION_LABS, "VIPS_INTERPRETATION_LABS", "labs"},
			{VIPS_INTERPRETATION_sRGB, "VIPS_INTERPRETATION_sRGB", "srgb"},
			{VIPS_INTERPRETATION_YXY, "VIPS_INTERPRETATION_YXY", "yxy"},
			{VIPS_INTERPRETATION_RGB16, "VIPS_INTERPRETATION_RGB16", "rgb16"},
			{VIPS_INTERPRETATION_GREY16, "VIPS_INTERPRETATION_GREY16", "grey16"},
			{0, NULL, NULL}
		};
		
		etype = g_enum_register_static( "VipsInterpretation", values );
	}

	return( etype );
}
GType
vips_band_format_get_type( void )
{
	static GType etype = 0;

	if( etype == 0 ) {
		static const GEnumValue values[] = {
			{VIPS_FORMAT_NOTSET, "VIPS_FORMAT_NOTSET", "notset"},
			{VIPS_FORMAT_UCHAR, "VIPS_FORMAT_UCHAR", "uchar"},
			{VIPS_FORMAT_CHAR, "VIPS_FORMAT_CHAR", "char"},
			{VIPS_FORMAT_USHORT, "VIPS_FORMAT_USHORT", "ushort"},
			{VIPS_FORMAT_SHORT, "VIPS_FORMAT_SHORT", "short"},
			{VIPS_FORMAT_UINT, "VIPS_FORMAT_UINT", "uint"},
			{VIPS_FORMAT_INT, "VIPS_FORMAT_INT", "int"},
			{VIPS_FORMAT_FLOAT, "VIPS_FORMAT_FLOAT", "float"},
			{VIPS_FORMAT_COMPLEX, "VIPS_FORMAT_COMPLEX", "complex"},
			{VIPS_FORMAT_DOUBLE, "VIPS_FORMAT_DOUBLE", "double"},
			{VIPS_FORMAT_DPCOMPLEX, "VIPS_FORMAT_DPCOMPLEX", "dpcomplex"},
			{VIPS_FORMAT_LAST, "VIPS_FORMAT_LAST", "last"},
			{0, NULL, NULL}
		};
		
		etype = g_enum_register_static( "VipsBandFormat", values );
	}

	return( etype );
}
GType
vips_coding_get_type( void )
{
	static GType etype = 0;

	if( etype == 0 ) {
		static const GEnumValue values[] = {
			{VIPS_CODING_NONE, "VIPS_CODING_NONE", "none"},
			{VIPS_CODING_LABQ, "VIPS_CODING_LABQ", "labq"},
			{VIPS_CODING_RAD, "VIPS_CODING_RAD", "rad"},
			{0, NULL, NULL}
		};
		
		etype = g_enum_register_static( "VipsCoding", values );
	}

	return( etype );
}
/* enumerations from "../../libvips/include/vips/object.h" */
GType
vips_argument_flags_get_type( void )
{
	static GType etype = 0;

	if( etype == 0 ) {
		static const GEnumValue values[] = {
			{VIPS_ARGUMENT_NONE, "VIPS_ARGUMENT_NONE", "none"},
			{VIPS_ARGUMENT_REQUIRED, "VIPS_ARGUMENT_REQUIRED", "required"},
			{VIPS_ARGUMENT_CONSTRUCT, "VIPS_ARGUMENT_CONSTRUCT", "construct"},
			{VIPS_ARGUMENT_SET_ONCE, "VIPS_ARGUMENT_SET_ONCE", "set-once"},
			{VIPS_ARGUMENT_INPUT, "VIPS_ARGUMENT_INPUT", "input"},
			{VIPS_ARGUMENT_OUTPUT, "VIPS_ARGUMENT_OUTPUT", "output"},
			{0, NULL, NULL}
		};
		
		etype = g_enum_register_static( "VipsArgumentFlags", values );
	}

	return( etype );
}

/* Generated data ends here */


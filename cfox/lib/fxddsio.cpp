/********************************************************************************
*                                                                               *
*                          D D S   I n p u t / O u t p u t                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXhalf.h"
#include "FXElement.h"
#include "FXStream.h"

/*
  Notes:
  - For cubic environment maps, one or more faces of a cube are written to the file, using either uncompressed or
    compressed formats, and all faces must be the same size. Each face can have mipmaps defined, although all faces
    must have the same number of mipmap levels. If a file contains a cube map, DDSCAPS_COMPLEX, DDSCAPS2_CUBEMAP,
    and one or more of DSCAPS2_CUBEMAP_POSITIVEX/Y/Z and/or DDSCAPS2_CUBEMAP_NEGATIVEX/Y/Z should be set.
    The faces are written in the order: positive x, negative x, positive y, negative y, positive z, negative z,
    with any missing faces omitted. Each face is written with its main image, followed by any mipmap levels.
  - For a volume texture, use the DDSCAPS_COMPLEX, DDSCAPS2_VOLUME, DDSD_DEPTH, flags and set and dwDepth.
    A volume texture is an extension of a standard texture for Direct3D 9; a volume texture is can be defined with
    or without mipmaps.  For volumes without mipmaps, each depth slice is written to the file in order.
    If mipmaps are included, all depth slices for a given mipmap level are written together, with each level
    containing half as many slices as the previous level with a minimum of 1.
  - Volume textures do not support compression in Direct3D 9.
  - For an uncompressed texture, use the DDSD_PITCH and DDPF_RGB flags; for a compressed texture,
    use the DDSD_LINEARSIZE and DDPF_FOURCC flags.
  - For a mipmapped texture, use the DDSD_MIPMAPCOUNT, DDSCAPS_MIPMAP, and DDSCAPS_COMPLEX flags also as
    well as the mipmap count member. If mipmaps are generated, all levels down to 1-by-1 are usually written.
*/



// Magic file header constant
#define DDSD_MAGIC                   0x20534444

// DDSHeader flags
#define DDSD_CAPS                    0x00000001
#define DDSD_HEIGHT                  0x00000002
#define DDSD_WIDTH                   0x00000004
#define DDSD_PITCH                   0x00000008
#define DDSD_PIXELFORMAT	     0x00001000
#define DDSD_MIPMAPCOUNT	     0x00020000
#define DDSD_LINEARSIZE   	     0x00080000
#define DDSD_DEPTH      	     0x00800000

// DDSPixelFormat flags
#define DDPF_ALPHAPIXELS	     0x00000001
#define DDPF_ALPHA                   0x00000002
#define DDPF_FOURCC                  0x00000004
#define DDPF_PALETTEINDEXED4         0x00000008
#define DDPF_PALETTEINDEXED8         0x00000020
#define DDPF_RGB                     0x00000040
#define DDPF_COMPRESSED              0x00000080
#define DDPF_RGBTOYUV                0x00000100
#define DDPF_YUV                     0x00000200
#define DDPF_ZBUFFER                 0x00000400
#define DDPF_PALETTEINDEXED1         0x00000800
#define DDPF_PALETTEINDEXED2         0x00001000
#define DDPF_ZPIXELS                 0x00002000
#define DDPF_STENCILBUFFER           0x00004000
#define DDPF_ALPHAPREMULT            0x00008000
#define DDPF_LUMINANCE               0x00020000
#define DDPF_BUMPLUMINANCE           0x00040000
#define DDPF_NORMAL                  0x80000000

// DDSHeader CAPS field
#define DDSCAPS_COMPLEX              0x00000008
#define DDSCAPS_TEXTURE              0x00001000
#define DDSCAPS_MIPMAP               0x00400000

// DDSHeader CAPS2 field
#define DDSCAPS2_CUBEMAP             0x00000200
#define DDSCAPS2_VOLUME              0x00200000

// DDSHeader Cube maps
#define DDSCAPS2_CUBEMAP_POSITIVEX   0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX   0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY   0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY   0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ   0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ   0x00008000


// DDSPixelFormat FOURCC constants (Compressed texture formats)
#define D3DFMT_DXT1                  0x31545844         // DXT1
#define D3DFMT_DXT2                  0x32545844         // DXT2
#define D3DFMT_DXT3                  0x33545844         // DXT3
#define D3DFMT_DXT4                  0x34545844         // DXT4
#define D3DFMT_DXT5                  0x35545844         // DXT5

// DDSPixelFormat FOURCC constants (Nonstandard formats)
#define D3DFMT_DX10                  0x30315844         // DX10
#define D3DFMT_RXGB                  0x42475852         // RXGB (AKA DOOM III)
#define D3DFMT_ATI1                  0x31495441         // ATI1
#define D3DFMT_ATI2                  0x32495441         // ATI2 (AKA 3Dc)
#define D3DFMT_A2XY                  0x59583241         // A2XY
#define D3DFMT_UYVY                  0x59565955         // UYVY format
#define D3DFMT_YUY2                  0x32595559         // YUY2 format
#define D3DFMT_R8G8_B8G8             0x47424752         // Pairs of pixels, [RG][BG] consecutive pixels share R and G (G0R0,G1B0, G2R2,G3B2) etc.
#define D3DFMT_G8R8_G8B8             0x42475247         // Pairs of pixels, [GR][GB] consecutive pixels share R and B (R0G0,B0G1, R2G2,B2G3) etc.

// DDSPixelFormat FOURCC constants (Floating point)
#define D3DFMT_R16F                  111                // 16-bit float format using 16 bits for the red channel.
#define D3DFMT_G16R16F               112                // 32-bit float format using 16 bits for the red channel and 16 bits for the green channel.
#define D3DFMT_A16B16G16R16F         113                // 64-bit float format using 16 bits for the each channel (alpha, blue, green, red).
#define D3DFMT_R32F                  114                // 32-bit float format using 32 bits for the red channel.
#define D3DFMT_G32R32F               115                // 64-bit float format using 32 bits for the red channel and 32 bits for the green channel.
#define D3DFMT_A32B32G32R32F         116                // 128-bit float format using 32 bits for the each channel (alpha, blue, green, red).

// DDSPixelFormat FOURCC constants (Signed integer formats)
#define D3DFMT_V8U8                  60                 // 16-bit bump-map format using 8 bits each for u and v data.
#define D3DFMT_L6V5U5                61                 // 16-bit bump-map format with luminance using 6 bits for luminance, and 5 bits each for v and u.
#define D3DFMT_X8L8V8U8              62                 // 32-bit bump-map format with luminance using 8 bits for each channel.
#define D3DFMT_Q8W8V8U8              63                 // 32-bit bump-map format using 8 bits for each channel.
#define D3DFMT_V16U16                64                 // 32-bit bump-map format using 16 bits for each channel.
#define D3DFMT_A2W10V10U10           67                 // 32-bit bump-map format using 2 bits for alpha and 10 bits each for w, v, and u.
#define D3DFMT_Q16W16V16U16          110                // 64-bit bump-map format using 16 bits for each component.
#define D3DFMT_CxV8U8                117                // 16-bit normal compression format. The texture sampler computes the C channel from: C = sqrt(1 - U2 - V2).

// DDSPixelFormat FOURCC constants (Unsigned integer formats)
#define D3DFMT_R8G8B8                20                 // 24-bit RGB pixel format with 8 bits per channel.
#define D3DFMT_A8R8G8B8              21                 // 32-bit ARGB pixel format with alpha, using 8 bits per channel.
#define D3DFMT_X8R8G8B8              22                 // 32-bit RGB pixel format, where 8 bits are reserved for each color.
#define D3DFMT_R5G6B5                23                 // 16-bit RGB pixel format with 5 bits for red, 6 bits for green, and 5 bits for blue.
#define D3DFMT_X1R5G5B5              24                 // 16-bit pixel format where 5 bits are reserved for each color.
#define D3DFMT_A1R5G5B5              25                 // 16-bit pixel format where 5 bits are reserved for each color and 1 bit is reserved for alpha.
#define D3DFMT_A4R4G4B4              26                 // 16-bit ARGB pixel format with 4 bits for each channel.
#define D3DFMT_R3G3B2                27                 // 8-bit RGB texture format using 3 bits for red, 3 bits for green, and 2 bits for blue.
#define D3DFMT_A8                    28                 // 8-bit alpha only.
#define D3DFMT_A8R3G3B2              29                 // 16-bit ARGB texture format using 8 bits for alpha, 3 bits each for red and green, and 2 bits for blue.
#define D3DFMT_X4R4G4B4              30                 // 16-bit RGB pixel format using 4 bits for each color.
#define D3DFMT_A2B10G10R10           31                 // 32-bit pixel format using 10 bits for each color and 2 bits for alpha.
#define D3DFMT_A8B8G8R8              32                 // 32-bit ARGB pixel format with alpha, using 8 bits per channel.
#define D3DFMT_X8B8G8R8              33                 // 32-bit RGB pixel format, where 8 bits are reserved for each color.
#define D3DFMT_G16R16                34                 // 32-bit pixel format using 16 bits each for green and red.
#define D3DFMT_A2R10G10B10           35                 // 32-bit pixel format using 10 bits each for red, green, and blue, and 2 bits for alpha.
#define D3DFMT_A16B16G16R16          36                 // 64-bit pixel format using 16 bits for each component.
#define D3DFMT_A8P8                  40                 // 8-bit color indexed with 8 bits of alpha.
#define D3DFMT_P8                    41                 // 8-bit color indexed.
#define D3DFMT_L8                    50                 // 8-bit luminance only.
#define D3DFMT_A8L8                  51                 // 16-bit using 8 bits each for alpha and luminance.
#define D3DFMT_A4L4                  52                 // 8-bit using 4 bits each for alpha and luminance.
#define D3DFMT_L16                   81                 // 16-bit luminance only.
#define D3DFMT_A1                    118                // 1-bit monochrome.
#define D3DFMT_A2B10G10R10_XR_BIAS   119                // 2.8-biased fixed point.
#define D3DFMT_BINARYBUFFER          199                // Binary format indicating that the data has no inherent type.

// DDSPixelFormat FOURCC constants (Buffer formats)
#define D3DFMT_D16_LOCKABLE          70                 // 16-bit z-buffer bit depth.
#define D3DFMT_D32                   71                 // 32-bit z-buffer bit depth.
#define D3DFMT_D15S1                 73                 // 16-bit z-buffer bit depth where 15 bits are reserved for the depth channel and 1 bit is reserved for the stencil channel.
#define D3DFMT_D24S8                 75                 // 32-bit z-buffer bit depth using 24 bits for the depth channel and 8 bits for the stencil channel.
#define D3DFMT_D24X8                 77                 // 32-bit z-buffer bit depth using 24 bits for the depth channel.
#define D3DFMT_D24X4S4               79                 // 32-bit z-buffer bit depth using 24 bits for the depth channel and 4 bits for the stencil channel.
#define D3DFMT_D16                   80                 // 16-bit z-buffer bit depth.
#define D3DFMT_D32F_LOCKABLE         82                 // A lockable format where the depth value is represented as a standard IEEE floating-point number.
#define D3DFMT_D24FS8                83                 // A non-lockable format that contains 24 bits of depth (in a 24-bit floating point format - 20e4) and 8 bits of stencil.
#define D3DFMT_D32_LOCKABLE          84                 // A lockable 32-bit depth buffer.
#define D3DFMT_S8_LOCKABLE           85                 // A lockable 8-bit stencil buffer.
#define D3DFMT_VERTEXDATA            100                // Describes a vertex buffer surface.
#define D3DFMT_INDEX16               101                // 16-bit index buffer bit depth.
#define D3DFMT_INDEX32               102                // 32-bit index buffer bit depth.

// DDSXHeader Resource dimension
#define D3D10_RESOURCE_DIMENSION_UNKNOWN        0               // Resource is of unknown type.
#define D3D10_RESOURCE_DIMENSION_BUFFER         1               // Resource is a buffer.
#define D3D10_RESOURCE_DIMENSION_TEXTURE1D      2               // Resource is a 1D texture.
#define D3D10_RESOURCE_DIMENSION_TEXTURE2D      3               // Resource is a 2D texture.
#define D3D10_RESOURCE_DIMENSION_TEXTURE3D      4               // Resource is a 3D texture.

// DDSXHeader Miscellaneous flag
#define D3D10_RESOURCE_MISC_GENERATE_MIPS       0x1             // Enable mipmap generation
#define D3D10_RESOURCE_MISC_SHARED              0x2             // Enable share
#define D3D10_RESOURCE_MISC_TEXTURECUBE         0x4             // Enable cube map
#define D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX   0x10
#define D3D10_RESOURCE_MISC_GDI_COMPATIBLE      0x20

// DDSXHeader Formats
#define DXGI_FORMAT_UNKNOWN                     0               // The format is not known.

#define DXGI_FORMAT_R32G32B32A32_TYPELESS       1               // A four-component, 128-bit typeless format.
#define DXGI_FORMAT_R32G32B32A32_FLOAT          2               // A four-component, 128-bit floating-point format
#define DXGI_FORMAT_R32G32B32A32_UINT           3               // A four-component, 128-bit unsigned-integer format
#define DXGI_FORMAT_R32G32B32A32_SINT           4               // A four-component, 128-bit signed-integer format.

#define DXGI_FORMAT_R32G32B32_TYPELESS          5               // A three-component, 96-bit typeless format.
#define DXGI_FORMAT_R32G32B32_FLOAT             6               // A three-component, 96-bit floating-point format.
#define DXGI_FORMAT_R32G32B32_UINT              7               // A three-component, 96-bit unsigned-integer format.
#define DXGI_FORMAT_R32G32B32_SINT              8               // A three-component, 96-bit signed-integer format.

#define DXGI_FORMAT_R16G16B16A16_TYPELESS       9               // A four-component, 64-bit typeless format.
#define DXGI_FORMAT_R16G16B16A16_FLOAT          10              // A four-component, 64-bit floating-point format.
#define DXGI_FORMAT_R16G16B16A16_UNORM          11              // A four-component, 64-bit unsigned-integer format.
#define DXGI_FORMAT_R16G16B16A16_UINT           12              // A four-component, 64-bit unsigned-integer format.
#define DXGI_FORMAT_R16G16B16A16_SNORM          13              // A four-component, 64-bit signed-integer format.
#define DXGI_FORMAT_R16G16B16A16_SINT           14              // A four-component, 64-bit signed-integer format.

#define DXGI_FORMAT_R32G32_TYPELESS             15              // A two-component, 64-bit typeless format.
#define DXGI_FORMAT_R32G32_FLOAT                16              // A two-component, 64-bit floating-point format.
#define DXGI_FORMAT_R32G32_UINT                 17              // A two-component, 64-bit unsigned-integer format.
#define DXGI_FORMAT_R32G32_SINT                 18              // A two-component, 64-bit signed-integer format.

#define DXGI_FORMAT_R32G8X24_TYPELESS           19              // A two-component, 64-bit typeless format.
#define DXGI_FORMAT_D32_FLOAT_S8X24_UINT        20              // A 32-bit floating-point component, and two unsigned-integer components (with an additional 32 bits).
#define DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS    21              // A 32-bit floating-point component, and two typeless components (with an additional 32 bits).
#define DXGI_FORMAT_X32_TYPELESS_G8X24_UINT     22              // A 32-bit typeless component, and two unsigned-integer components (with an additional 32 bits).

#define DXGI_FORMAT_R10G10B10A2_TYPELESS        23              // A four-component, 32-bit typeless format.
#define DXGI_FORMAT_R10G10B10A2_UNORM           24              // A four-component, 32-bit unsigned-integer format.
#define DXGI_FORMAT_R10G10B10A2_UINT            25              // A four-component, 32-bit unsigned-integer format.

#define DXGI_FORMAT_R11G11B10_FLOAT             26              // A three-component, 32-bit floating-point format.

#define DXGI_FORMAT_R8G8B8A8_TYPELESS           27              // A three-component, 32-bit typeless format.
#define DXGI_FORMAT_R8G8B8A8_UNORM              28              // A four-component, 32-bit unsigned-integer format.
#define DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         29              // A four-component, 32-bit unsigned-normalized integer sRGB format.
#define DXGI_FORMAT_R8G8B8A8_UINT               30              // A four-component, 32-bit unsigned-integer format.
#define DXGI_FORMAT_R8G8B8A8_SNORM              31              // A three-component, 32-bit signed-integer format.
#define DXGI_FORMAT_R8G8B8A8_SINT               32              // A three-component, 32-bit signed-integer format.

#define DXGI_FORMAT_R16G16_TYPELESS             33              // A two-component, 32-bit typeless format.
#define DXGI_FORMAT_R16G16_FLOAT                34              // A two-component, 32-bit floating-point format.
#define DXGI_FORMAT_R16G16_UNORM                35              // A two-component, 32-bit unsigned-integer format.
#define DXGI_FORMAT_R16G16_UINT                 36              // A two-component, 32-bit unsigned-integer format.
#define DXGI_FORMAT_R16G16_SNORM                37              // A two-component, 32-bit signed-integer format.
#define DXGI_FORMAT_R16G16_SINT                 38              // A two-component, 32-bit signed-integer format.

#define DXGI_FORMAT_R32_TYPELESS                39              // A single-component, 32-bit typeless format.
#define DXGI_FORMAT_D32_FLOAT                   40              // A single-component, 32-bit floating-point format.
#define DXGI_FORMAT_R32_FLOAT                   41              // A single-component, 32-bit floating-point format.
#define DXGI_FORMAT_R32_UINT                    42              // A single-component, 32-bit unsigned-integer format.
#define DXGI_FORMAT_R32_SINT                    43              // A single-component, 32-bit signed-integer format.

#define DXGI_FORMAT_R24G8_TYPELESS              44              // A two-component, 32-bit typeless format.
#define DXGI_FORMAT_D24_UNORM_S8_UINT           45              // A 32-bit z-buffer format that uses 24 bits for the depth channel and 8 bits for the stencil channel.
#define DXGI_FORMAT_R24_UNORM_X8_TYPELESS       46              // A 32-bit format, that contains a 24 bit, single-component, unsigned-normalized integer, with an additional typeless 8 bits.
#define DXGI_FORMAT_X24_TYPELESS_G8_UINT        47              // A 32-bit format, that contains a 24 bit, single-component, typeless format, with an additional 8 bit unsigned integer component.

#define DXGI_FORMAT_R8G8_TYPELESS               48              // A two-component, 16-bit typeless format.
#define DXGI_FORMAT_R8G8_UNORM                  49              // A two-component, 16-bit unsigned-integer format.
#define DXGI_FORMAT_R8G8_UINT                   50              // A two-component, 16-bit unsigned-integer format.
#define DXGI_FORMAT_R8G8_SNORM                  51              // A two-component, 16-bit signed-integer format.
#define DXGI_FORMAT_R8G8_SINT                   52              // A two-component, 16-bit signed-integer format.

#define DXGI_FORMAT_R16_TYPELESS                53              // A single-component, 16-bit typeless format.
#define DXGI_FORMAT_R16_FLOAT                   54              // A single-component, 16-bit floating-point format.
#define DXGI_FORMAT_D16_UNORM                   55              // A single-component, 16-bit unsigned-normalized integer format.
#define DXGI_FORMAT_R16_UNORM                   56              // A single-component, 16-bit unsigned-integer format.
#define DXGI_FORMAT_R16_UINT                    57              // A single-component, 16-bit unsigned-integer format.
#define DXGI_FORMAT_R16_SNORM                   58              // A single-component, 16-bit signed-integer format.
#define DXGI_FORMAT_R16_SINT                    59              // A single-component, 16-bit signed-integer format.

#define DXGI_FORMAT_R8_TYPELESS                 60              // A single-component, 8-bit typeless format.
#define DXGI_FORMAT_R8_UNORM                    61              // A single-component, 8-bit unsigned-integer format.
#define DXGI_FORMAT_R8_UINT                     62              // A single-component, 8-bit unsigned-integer format.
#define DXGI_FORMAT_R8_SNORM                    63              // A single-component, 8-bit signed-integer format.
#define DXGI_FORMAT_R8_SINT                     64              // A single-component, 8-bit signed-integer format.
#define DXGI_FORMAT_A8_UNORM                    65              // A single-component, 8-bit unsigned-integer format.

#define DXGI_FORMAT_R1_UNORM                    66              // A single-component, 1-bit unsigned-normalized integer format.

#define DXGI_FORMAT_R9G9B9E5_SHAREDEXP          67              // A four-component, 32-bit floating-point format.

#define DXGI_FORMAT_R8G8_B8G8_UNORM             68              // A four-component, 32-bit unsigned-normalized integer format.
#define DXGI_FORMAT_G8R8_G8B8_UNORM             69              // A four-component, 32-bit unsigned-normalized integer format.

#define DXGI_FORMAT_BC1_TYPELESS                70              // 4-channel typeless block-compression format.
#define DXGI_FORMAT_BC1_UNORM                   71              // 4-channel block-compression format.
#define DXGI_FORMAT_BC1_UNORM_SRGB              72              // 4-channel block-compression format for sRGB data.

#define DXGI_FORMAT_BC2_TYPELESS                73              // 4-channel typeless block-compression format.
#define DXGI_FORMAT_BC2_UNORM                   74              // 4-channel block-compression format.
#define DXGI_FORMAT_BC2_UNORM_SRGB              75              // 4-channel block-compression format for sRGB data.

#define DXGI_FORMAT_BC3_TYPELESS                76              // 4-channel typeless block-compression format.
#define DXGI_FORMAT_BC3_UNORM                   77              // 4-channel block-compression format.
#define DXGI_FORMAT_BC3_UNORM_SRGB              78              // 4-channel block-compression format for sRGB data.

#define DXGI_FORMAT_BC4_TYPELESS                79              // 1-channel typeless block-compression format.
#define DXGI_FORMAT_BC4_UNORM                   80              // 1-channel block-compression format.
#define DXGI_FORMAT_BC4_SNORM                   81              // 1-channel block-compression format.

#define DXGI_FORMAT_BC5_TYPELESS                82              // 2-channel typeless block-compression format.
#define DXGI_FORMAT_BC5_UNORM                   83              // 2-channel block-compression format.
#define DXGI_FORMAT_BC5_SNORM                   84              // 2-channel block-compression format.

#define DXGI_FORMAT_B5G6R5_UNORM                85              // A three-component, 16-bit unsigned-normalized integer format.
#define DXGI_FORMAT_B5G5R5A1_UNORM              86              // A four-component, 16-bit unsigned-normalized integer format that supports 1-bit alpha.
#define DXGI_FORMAT_B8G8R8A8_UNORM              87              // A four-component, 16-bit unsigned-normalized integer format that supports 8-bit alpha.
#define DXGI_FORMAT_B8G8R8X8_UNORM              88              // A four-component, 16-bit unsigned-normalized integer format.
#define DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  89              // A four-component, 32-bit format that supports 2-bit alpha.
#define DXGI_FORMAT_B8G8R8A8_TYPELESS           90              // A four-component, 32-bit typeless format that supports 8-bit alpha.
#define DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         91              // A four-component, 32-bit unsigned-normalized standard RGB format that supports 8-bit alpha.
#define DXGI_FORMAT_B8G8R8X8_TYPELESS           92              // A four-component, 32-bit typeless format.
#define DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         93              // A four-component, 32-bit unsigned-normalized standard RGB format.
#define DXGI_FORMAT_BC6H_TYPELESS               94              // A typeless block-compression format
#define DXGI_FORMAT_BC6H_UF16                   95              // A block-compression format.
#define DXGI_FORMAT_BC6H_SF16                   96              // A block-compression format.
#define DXGI_FORMAT_BC7_TYPELESS                97              // A typeless block-compression format.
#define DXGI_FORMAT_BC7_UNORM                   98              // A block-compression format.
#define DXGI_FORMAT_BC7_UNORM_SRGB              99              // A block-compression format.

#define DXGI_FORMAT_AYUV                        100             // Most common YUV 4:4:4 video resource format
#define DXGI_FORMAT_Y410                        101             // 10-bit per channel packed YUV 4:4:4 video resource format
#define DXGI_FORMAT_Y416                        102             // 16-bit per channel packed YUV 4:4:4 video resource format
#define DXGI_FORMAT_NV12                        103             // Most common YUV 4:2:0 video resource format
#define DXGI_FORMAT_P010                        104             // 10-bit per channel planar YUV 4:2:0 video resource format
#define DXGI_FORMAT_P016                        105             // 16-bit per channel planar YUV 4:2:0 video resource format
#define DXGI_FORMAT_420_OPAQUE                  106             // 8-bit per channel planar YUV 4:2:0 video resource format
#define DXGI_FORMAT_YUY2                        107             // Most common YUV 4:2:2 video resource format
#define DXGI_FORMAT_Y210                        108             // 10-bit per channel packed YUV 4:2:2 video resource format
#define DXGI_FORMAT_Y216                        109             // 16-bit per channel packed YUV 4:2:2 video resource format.
#define DXGI_FORMAT_NV11                        110             // Most common planar YUV 4:1:1 video resource format
#define DXGI_FORMAT_AI44                        111             // 4-bit palletized YUV format that is commonly used for DVD subpicture
#define DXGI_FORMAT_IA44                        112             // 4-bit palletized YUV format that is commonly used for DVD subpicture
#define DXGI_FORMAT_P8                          113             // 88-bit palletized format that is used for palletized RGB data when the processor processes ISDB-T data and for palletized YUV data when the processor processes BluRay data
#define DXGI_FORMAT_A8P8                        114             // 8-bit palletized format with 8 bits of alpha that is used for palletized YUV data when the processor processes BluRay data
#define DXGI_FORMAT_B4G4R4A4_UNORM              115             // A four-component, 16-bit unsigned-normalized integer format that supports 4 bits for each channel including alpha


// Internal codes
#define CODE_NONE       0
#define CODE_DXT1       1
#define CODE_DXT2       2
#define CODE_DXT3       3
#define CODE_DXT4       4
#define CODE_DXT5       5

using namespace FX;

/*******************************************************************************/

namespace FX {

#ifndef FXLOADDDS
extern FXAPI FXbool fxcheckDDS(FXStream& store);
extern FXAPI FXbool fxloadDDS(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& depth);
extern FXAPI FXbool fxsaveDDS(FXStream& store,FXColor* data,FXint width,FXint height,FXint depth);
#endif


// Pixel format
struct DDSPixelFormat {
  FXuint         dwSize;
  FXuint         dwFlags;
  FXuint         dwFourCC;
  FXuint         dwRGBBitCount;
  FXuint         dwRBitMask;
  FXuint         dwGBitMask;
  FXuint         dwBBitMask;
  FXuint         dwABitMask;
  };


// DDS Header
struct DDSHeader {
  FXuint         dwSize;
  FXuint         dwFlags;
  FXuint         dwHeight;
  FXuint         dwWidth;
  FXuint         dwLinearSize;
  FXuint         dwDepth;
  FXuint         dwMipMapCount;
  FXuint         dwReserved1[11];
  DDSPixelFormat ddpf;
  FXuint         dwCaps;
  FXuint         dwCaps2;
  FXuint         dwCaps3;
  FXuint         dwCaps4;
  FXuint         dwReserved2;
  };


// DX10 extra header structure
struct DDSXHeader {
  FXuint         dxgiFormat;
  FXuint         resourceDimension;
  FXuint         miscFlag;
  FXuint         arraySize;
  FXuint         reserved;
  };


// DDS Image
struct DDSImage {
  FXuint         magic;
  DDSHeader      header;
  DDSXHeader     xheader;
  FXuint         size;
  FXuchar       *data;
  };


// Fast integer square root
static FXuint isqrt(FXuint val){
  FXuint temp,g=0,b=0x8000,bshft=15;
  do{if(val>=(temp=(((g<<1)+b)<<bshft--))){g+=b;val-=temp;}}while(b>>=1);
  return g;
  }


// Undo premultiplied alpha
// The math: 255*X = (255*R * 255*A)/255, so 255*R = 255*X*255 / 255*A
static void dds_correct_color(FXuchar *image,FXuint size){
  FXuint i,a;
  for(i=0; i<size; i+=4){
    if((a=image[i+3])>0){
      image[i+0]=(image[i+0]*255)/a;
      image[i+1]=(image[i+1]*255)/a;
      image[i+2]=(image[i+2]*255)/a;
      }
    }
  }


// Swizzle red and alpha for RXGB
static void dds_correct_swizzle(FXuchar *image,FXuint size){
  FXuint i;
  for(i=0; i<size; i+=4){
    image[i+2]^=image[i+3];
    image[i+3]^=image[i+2];
    image[i+2]^=image[i+3];
    }
  }


// Decompress DXT1/BC1 image
static FXbool dds_decompress_DXT1(const DDSImage& dds,FXuchar *image){
  const FXuchar *temp=dds.data;
  FXuint x,y,z,i,j,select,bitmask,offset;
  FXuchar r0,g0,b0,r1,g1,b1;
  FXushort c0,c1;
  FXuchar colors[4][4];

  FXTRACE((100,"dds_decompress_DXT1\n"));

  // Loop over 4x4 blocks
  for(z=0; z<dds.header.dwDepth; z+=1){
    for(y=0; y<dds.header.dwHeight; y+=4){
      for(x=0; x<dds.header.dwWidth; x+=4){

        // Grab two 5,6,5 colors
        c0=(((FXushort)temp[1])<<8) | (FXushort)temp[0];
        c1=(((FXushort)temp[3])<<8) | (FXushort)temp[2];

        r0=(c0>>11)&0x1f;
        g0=(c0>>5)&0x3f;
        b0=c0&0x1f;

        r1=(c1>>11)&0x1f;
        g1=(c1>>5)&0x3f;
        b1=c1&0x1f;

        colors[0][0]=(b0<<3)|(b0>>2);     // Convert from 5,6,5 to 8,8,8 color #0
        colors[0][1]=(g0<<2)|(g0>>4);
        colors[0][2]=(r0<<3)|(r0>>2);
        colors[0][3]=0xFF;

        colors[1][0]=(b1<<3)|(b1>>2);     // Convert from 5,6,5 to 8,8,8 color #1
        colors[1][1]=(g1<<2)|(g1>>4);
        colors[1][2]=(r1<<3)|(r1>>2);
        colors[1][3]=0xFF;

        // Four color block: 00=color #0, 01=color #1, 10=color #2, 11=color #3
        if(c0>c1){
          colors[2][0]=(2*colors[0][0]+colors[1][0]+1)/3;
          colors[2][1]=(2*colors[0][1]+colors[1][1]+1)/3;
          colors[2][2]=(2*colors[0][2]+colors[1][2]+1)/3;
          colors[2][3]=255;
          colors[3][0]=(colors[0][0]+2*colors[1][0]+1)/3;
          colors[3][1]=(colors[0][1]+2*colors[1][1]+1)/3;
          colors[3][2]=(colors[0][2]+2*colors[1][2]+1)/3;
          colors[3][3]=255;
          }

        // Three color block: 00=color #0, 01=color #1, 10=color #2, 11=transparent
        else{
          colors[2][0]=(colors[0][0]+colors[1][0])/2;
          colors[2][1]=(colors[0][1]+colors[1][1])/2;
          colors[2][2]=(colors[0][2]+colors[1][2])/2;
          colors[2][3]=255;
          colors[3][0]=0;
          colors[3][1]=0;
          colors[3][2]=0;
          colors[3][3]=0;
          }

        // Get index bits all at once
        bitmask=(((FXuint)temp[7])<<24)|(((FXuint)temp[6])<<16)|(((FXuint)temp[5])<<8)|((FXuint)temp[4]);

        // Decode the bits
        for(j=0; j<4; ++j){
          for(i=0; i<4; ++i){
            if(((x+i)<dds.header.dwWidth) && ((y+j)<dds.header.dwHeight)){
              offset=((z*dds.header.dwHeight+y+j)*dds.header.dwWidth+x+i)<<2;
              select=bitmask&3;
              image[offset+0]=colors[select][0];
              image[offset+1]=colors[select][1];
              image[offset+2]=colors[select][2];
              image[offset+3]=colors[select][3];
              }
            bitmask>>=2;
            }
          }
        temp+=8;
        }
      }
    }
  return true;
  }


// Decompress DXT3/BC2 image
static FXbool dds_decompress_DXT3(const DDSImage& dds,FXuchar *image){
  FXuchar *temp=dds.data;
  FXuint x,y,z,i,j,select,bitmask,offset;
  FXuchar r0,g0,b0,r1,g1,b1;
  FXushort c0,c1;
  FXuchar colors[4][4];
  FXuchar alpha[4][4];

  FXTRACE((100,"dds_decompress_DXT3\n"));

  // Loop over 4x4 blocks
  for(z=0; z<dds.header.dwDepth; z+=1){
    for(y=0; y<dds.header.dwHeight; y+=4){
      for(x=0; x<dds.header.dwWidth; x+=4){

        // Grab 16 4-bit alpha values and convert them to 8-bit ones
        alpha[0][0]=(temp[0]&15)*17;
        alpha[0][1]=(temp[0]>>4)*17;
        alpha[0][2]=(temp[1]&15)*17;
        alpha[0][3]=(temp[1]>>4)*17;

        alpha[1][0]=(temp[2]&15)*17;
        alpha[1][1]=(temp[2]>>4)*17;
        alpha[1][2]=(temp[3]&15)*17;
        alpha[1][3]=(temp[3]>>4)*17;

        alpha[2][0]=(temp[4]&15)*17;
        alpha[2][1]=(temp[4]>>4)*17;
        alpha[2][2]=(temp[5]&15)*17;
        alpha[2][3]=(temp[5]>>4)*17;

        alpha[3][0]=(temp[6]&15)*17;
        alpha[3][1]=(temp[6]>>4)*17;
        alpha[3][2]=(temp[7]&15)*17;
        alpha[3][3]=(temp[7]>>4)*17;

        // Grab two 5,6,5 colors
        c0=(((FXushort)temp[9])<<8) | (FXushort)temp[8];
        c1=(((FXushort)temp[11])<<8) | (FXushort)temp[10];

        r0=(c0>>11)&0x1f;
        g0=(c0>>5)&0x3f;
        b0=c0&0x1f;

        r1=(c1>>11)&0x1f;
        g1=(c1>>5)&0x3f;
        b1=c1&0x1f;

        colors[0][0]=(b0<<3)|(b0>>2);     // Convert from 5,6,5 to 8,8,8 color #0
        colors[0][1]=(g0<<2)|(g0>>4);
        colors[0][2]=(r0<<3)|(r0>>2);
        colors[0][3]=0xFF;

        colors[1][0]=(b1<<3)|(b1>>2);     // Convert from 5,6,5 to 8,8,8 color #1
        colors[1][1]=(g1<<2)|(g1>>4);
        colors[1][2]=(r1<<3)|(r1>>2);
        colors[1][3]=0xFF;

        // Four color block: 00=color #0, 01=color #1, 10=color #2, 11=color #3
        colors[2][0]=(2*colors[0][0]+colors[1][0]+1)/3;
        colors[2][1]=(2*colors[0][1]+colors[1][1]+1)/3;
        colors[2][2]=(2*colors[0][2]+colors[1][2]+1)/3;
        colors[2][3]=0xFF;

        colors[3][0]=(colors[0][0]+2*colors[1][0]+1)/3;
        colors[3][1]=(colors[0][1]+2*colors[1][1]+1)/3;
        colors[3][2]=(colors[0][2]+2*colors[1][2]+1)/3;
        colors[3][3]=0xFF;

        // Get index bits all at once
        bitmask=(((FXuint)temp[15])<<24)|(((FXuint)temp[14])<<16)|(((FXuint)temp[13])<<8)|((FXuint)temp[12]);

        // Decode the bits
        for(j=0; j<4; ++j){
          for(i=0; i<4; ++i){
            if(((x+i)<dds.header.dwWidth) && ((y+j)<dds.header.dwHeight)){
              offset=((z*dds.header.dwHeight+y+j)*dds.header.dwWidth+x+i)<<2;
              select=bitmask&3;
              image[offset+0]=colors[select][0];
              image[offset+1]=colors[select][1];
              image[offset+2]=colors[select][2];
              image[offset+3]=alpha[j][i];
              }
            bitmask>>=2;
            }
          }
        temp+=16;
        }
      }
    }
  return true;
  }


// Decompress DXT2 image; has premultiplied alpha
static FXbool dds_decompress_DXT2(const DDSImage& dds,FXuchar *image){
  if(dds_decompress_DXT3(dds,image)){
    dds_correct_color(image,dds.header.dwWidth*dds.header.dwHeight*dds.header.dwDepth*4);
    return true;
    }
  return false;
  }


// Decompress DXT5/BC3 image
static FXbool dds_decompress_DXT5(const DDSImage& dds,FXuchar *image){
  FXuchar *temp=dds.data;
  FXuint x,y,z,i,j,select,bitmask,bits,offset;
  FXuchar r0,g0,b0,r1,g1,b1;
  FXushort c0,c1;
  FXuchar colors[4][4];
  FXuchar levels[8];
  FXuchar alpha[4][4];

  FXTRACE((150,"dds_decompress_DXT5\n"));

  // Loop over 4x4 blocks
  for(z=0; z<dds.header.dwDepth; z+=1){
    for(y=0; y<dds.header.dwHeight; y+=4){
      for(x=0; x<dds.header.dwWidth; x+=4){

        // Grab two 8-bit alphas
        levels[0]=temp[0];
        levels[1]=temp[1];

        // Six interpolated alpha levels
        if(levels[0]>levels[1]){
          levels[2]=(6*levels[0]+1*levels[1]+3)/7;                // bit code 010
          levels[3]=(5*levels[0]+2*levels[1]+3)/7;                // bit code 011
          levels[4]=(4*levels[0]+3*levels[1]+3)/7;                // bit code 100
          levels[5]=(3*levels[0]+4*levels[1]+3)/7;                // bit code 101
          levels[6]=(2*levels[0]+5*levels[1]+3)/7;                // bit code 110
          levels[7]=(1*levels[0]+6*levels[1]+3)/7;                // bit code 111
          }

        // 4 interpolated alpha levels
        else{
          levels[2]=(4*levels[0]+1*levels[1]+2)/5;                // Bit code 010
          levels[3]=(3*levels[0]+2*levels[1]+2)/5;                // Bit code 011
          levels[4]=(2*levels[0]+3*levels[1]+2)/5;                // Bit code 100
          levels[5]=(1*levels[0]+4*levels[1]+2)/5;                // Bit code 101
          levels[6]=0x00;                                         // Bit code 110
          levels[7]=0xFF;                                         // Bit code 111
          }

        // First three bytes
        bits=(((FXuint)temp[4])<<16)|(((FXuint)temp[3])<<8)|((FXuint)temp[2]);
        for(j=0; j<2; ++j){
          for(i=0; i<4; ++i){
            alpha[j][i]=levels[bits&7];
            bits>>=3;
            }
          }

        // Last three bytes
        bits=(((FXuint)temp[7])<<16)|(((FXuint)temp[6])<<8)|((FXuint)temp[5]);
        for(j=2; j<4; ++j){
          for(i=0; i<4; ++i){
            alpha[j][i]=levels[bits&7];
            bits>>=3;
            }
          }

        // Grab two 5,6,5 colors
        c0=(((FXushort)temp[9])<<8) | (FXushort)temp[8];
        c1=(((FXushort)temp[11])<<8) | (FXushort)temp[10];

        r0=(c0>>11)&0x1f;
        g0=(c0>>5)&0x3f;
        b0=c0&0x1f;

        r1=(c1>>11)&0x1f;
        g1=(c1>>5)&0x3f;
        b1=c1&0x1f;

        colors[0][0]=(b0<<3)|(b0>>2);     // Convert from 5,6,5 to 8,8,8 color #0
        colors[0][1]=(g0<<2)|(g0>>4);
        colors[0][2]=(r0<<3)|(r0>>2);
        colors[0][3]=0xFF;

        colors[1][0]=(b1<<3)|(b1>>2);     // Convert from 5,6,5 to 8,8,8 color #1
        colors[1][1]=(g1<<2)|(g1>>4);
        colors[1][2]=(r1<<3)|(r1>>2);
        colors[1][3]=0xFF;

        // Four color block: 00=color #0, 01=color #1, 10=color #2, 11=color #3
        colors[2][0]=(2*colors[0][0]+colors[1][0]+1)/3;
        colors[2][1]=(2*colors[0][1]+colors[1][1]+1)/3;
        colors[2][2]=(2*colors[0][2]+colors[1][2]+1)/3;
        colors[2][3]=0xFF;

        colors[3][0]=(colors[0][0]+2*colors[1][0]+1)/3;
        colors[3][1]=(colors[0][1]+2*colors[1][1]+1)/3;
        colors[3][2]=(colors[0][2]+2*colors[1][2]+1)/3;
        colors[3][3]=0xFF;

        // Get index bits all at once
        bitmask=(((FXuint)temp[15])<<24)|(((FXuint)temp[14])<<16)|(((FXuint)temp[13])<<8)|((FXuint)temp[12]);

        // Decode the bits
        for(j=0; j<4; ++j){
          for(i=0; i<4; ++i){
            if(((x+i)<dds.header.dwWidth) && ((y+j)<dds.header.dwHeight)){
              offset=((z*dds.header.dwHeight+y+j)*dds.header.dwWidth+x+i)<<2;
              select=bitmask&3;
              image[offset+0]=colors[select][0];
              image[offset+1]=colors[select][1];
              image[offset+2]=colors[select][2];
              image[offset+3]=alpha[j][i];
              }
            bitmask>>=2;
            }
          }
        temp+=16;
        }
      }
    }
  return true;
  }


// Decompress DXT4 image; has premultiplied alpha
static FXbool dds_decompress_DXT4(const DDSImage& dds,FXuchar *image){
  if(dds_decompress_DXT5(dds,image)){
    dds_correct_color(image,dds.header.dwWidth*dds.header.dwHeight*dds.header.dwDepth*4);
    return true;
    }
  return false;
  }


// Decompress RXGB image
static FXbool dds_decompress_RXGB(const DDSImage& dds,FXuchar *image){
  if(dds_decompress_DXT5(dds,image)){
    dds_correct_swizzle(image,dds.header.dwWidth*dds.header.dwHeight*dds.header.dwDepth*4);
    return true;
    }
  return false;
  }


// Decompress BC4 (ATI1) image
static FXbool dds_decompress_BC4(const DDSImage& dds,FXuchar *image){
  FXuchar *temp=dds.data;
  FXuint x,y,z,i,j,bits,offset;
  FXuchar levels[8];

  FXTRACE((150,"dds_decompress_BC4\n"));

  // Loop over 4x4 blocks
  for(z=0; z<dds.header.dwDepth; z+=1){
    for(y=0; y<dds.header.dwHeight; y+=4){
      for(x=0; x<dds.header.dwWidth; x+=4){

        // Grab two 8-bit grey levels
        levels[0]=temp[0];
        levels[1]=temp[1];

        // Six interpolated grey levels
        if(levels[0]>levels[1]){
          levels[2]=(6*levels[0]+1*levels[1]+3)/7;
          levels[3]=(5*levels[0]+2*levels[1]+3)/7;
          levels[4]=(4*levels[0]+3*levels[1]+3)/7;
          levels[5]=(3*levels[0]+4*levels[1]+3)/7;
          levels[6]=(2*levels[0]+5*levels[1]+3)/7;
          levels[7]=(1*levels[0]+6*levels[1]+3)/7;
          }

        // 4 interpolated grey levels
        else{
          levels[2]=(4*levels[0]+1*levels[1]+2)/5;
          levels[3]=(3*levels[0]+2*levels[1]+2)/5;
          levels[4]=(2*levels[0]+3*levels[1]+2)/5;
          levels[5]=(1*levels[0]+4*levels[1]+2)/5;
          levels[6]=0;
          levels[7]=255;
          }

        // First three bytes
        bits=(((FXuint)temp[4])<<16)|(((FXuint)temp[3])<<8)|((FXuint)temp[2]);
        for(j=0; j<2; ++j){
          for(i=0; i<4; ++i){
            if(((x+i)<dds.header.dwWidth) && ((y+j)<dds.header.dwHeight)){
              offset=((z*dds.header.dwHeight+y+j)*dds.header.dwWidth+x+i)<<2;
              image[offset+0]=image[offset+1]=image[offset+2]=image[offset+3]=levels[bits&7];
              }
            bits>>=3;
            }
          }

        // Last three bytes
        bits=(((FXuint)temp[7])<<16)|(((FXuint)temp[6])<<8)|((FXuint)temp[5]);
        for(j=2; j<4; ++j){
          for(i=0; i<4; ++i){
            if(((x+i)<dds.header.dwWidth) && ((y+j)<dds.header.dwHeight)){
              offset=((z*dds.header.dwHeight+y+j)*dds.header.dwWidth+x+i)<<2;
              image[offset+0]=image[offset+1]=image[offset+2]=image[offset+3]=levels[bits&7];
              }
            bits>>=3;
            }
          }
        temp+=8;
        }
      }
    }
  return true;
  }


// Decompress 3DC (ATI2) image
static FXbool dds_decompress_3DC(const DDSImage& dds,FXuchar *image){
  FXuchar *temp=dds.data;
  FXuint x,y,z,i,j,redbits,grnbits,offset;
  FXint tx,ty,t;
  FXuchar red[8];
  FXuchar grn[8];

  FXTRACE((150,"dds_decompress_3DC\n"));

  // Loop over 4x4 blocks
  for(z=0; z<dds.header.dwDepth; z+=1){
    for(y=0; y<dds.header.dwHeight; y+=4){
      for(x=0; x<dds.header.dwWidth; x+=4){

        // Grab two reds
        red[0]=temp[0];
        red[1]=temp[1];

        // Six interpolated values
        if(red[0]>red[1]){
          red[2]=(6*red[0]+1*red[1]+3)/7;
          red[3]=(5*red[0]+2*red[1]+3)/7;
          red[4]=(4*red[0]+3*red[1]+3)/7;
          red[5]=(3*red[0]+4*red[1]+3)/7;
          red[6]=(2*red[0]+5*red[1]+3)/7;
          red[7]=(1*red[0]+6*red[1]+3)/7;
          }

        // Four interpolated values
        else{
          red[2]=(4*red[0]+1*red[1]+2)/5;
          red[3]=(3*red[0]+2*red[1]+2)/5;
          red[4]=(2*red[0]+3*red[1]+2)/5;
          red[5]=(1*red[0]+4*red[1]+2)/5;
          red[6]=0;
          red[7]=255;
          }

        // Grab two greens
        grn[0]=temp[8];
        grn[1]=temp[9];

        // Six interpolated values
        if(grn[0]>grn[1]){
          grn[2]=(6*grn[0]+1*grn[1]+3)/7;
          grn[3]=(5*grn[0]+2*grn[1]+3)/7;
          grn[4]=(4*grn[0]+3*grn[1]+3)/7;
          grn[5]=(3*grn[0]+4*grn[1]+3)/7;
          grn[6]=(2*grn[0]+5*grn[1]+3)/7;
          grn[7]=(1*grn[0]+6*grn[1]+3)/7;
          }

        // Four interpolated values
        else{
          grn[2]=(4*grn[0]+1*grn[1]+2)/5;
          grn[3]=(3*grn[0]+2*grn[1]+2)/5;
          grn[4]=(2*grn[0]+3*grn[1]+2)/5;
          grn[5]=(1*grn[0]+4*grn[1]+2)/5;
          grn[6]=0;
          grn[7]=255;
          }

        // Decode the first 3 bytes
        redbits=(((FXuint)temp[4])<<16)|(((FXuint)temp[3])<<8)|((FXuint)temp[2]);
        grnbits=(((FXuint)temp[12])<<16)|(((FXuint)temp[11])<<8)|((FXuint)temp[10]);
        for(j=0; j<2; ++j){
          for(i=0; i<4; ++i){
            if(((x+i)<dds.header.dwWidth) && ((y+j)<dds.header.dwHeight)){
              offset=((z*dds.header.dwHeight+y+j)*dds.header.dwWidth+x+i)<<2;
              image[offset+1]=ty=grn[grnbits&7];
              image[offset+2]=tx=red[redbits&7];
              t=127*128-(tx-127)*(tx-128)-(ty-127)*(ty-128);
              if(t>0){
                image[offset+0]=(FXuchar)(isqrt(t)+128);
                }
              else{
                image[offset+0]=127;
                }
              image[offset+3]=255;
              }
            redbits>>=3;
            grnbits>>=3;
            }
          }

        // Decode the last 3 bytes
        redbits=(((FXuint)temp[7])<<16)|(((FXuint)temp[6])<<8)|((FXuint)temp[5]);
        grnbits=(((FXuint)temp[15])<<16)|(((FXuint)temp[14])<<8)|((FXuint)temp[13]);
        for(j=2; j<4; ++j){
          for(i=0; i<4; ++i){
            if(((x+i)<dds.header.dwWidth) && ((y+j)<dds.header.dwHeight)){
              offset=((z*dds.header.dwHeight+y+j)*dds.header.dwWidth+x+i)<<2;
              image[offset+2]=tx=red[redbits&7];
              image[offset+1]=ty=grn[grnbits&7];
              t=127*128-(tx-127)*(tx-128)-(ty-127)*(ty-128);
              if(t>0){
                image[offset+0]=(FXuchar)(isqrt(t)+128);
                }
              else{
                image[offset+0]=127;
                }
              image[offset+3]=255;
              }
            redbits>>=3;
            grnbits>>=3;
            }
          }
        temp+=16;
        }
      }
    }
  return true;
  }


// Compute shifts
static void getShifts(FXuint mask,FXuint& shift,FXuint& mul,FXuint& sc){
  FXuint bits=0;
  shift=0;
  mul=1;
  sc=0;
  while(!(mask&1)){
    mask>>=1;
    shift++;
    }
  while(mask&(1<<bits)) bits++;
  while((mask*mul)<255){
    mul=(mul<<bits)+1;
    }
  mask*=mul;
  while((mask&~0xff)!=0){
    mask>>=1;
    sc++;
    }
  }


// General decompress integer pixel
static FXbool dds_decompress_RGB(const DDSImage& dds,FXuchar *image,FXuint bmask,FXuint gmask,FXuint rmask,FXuint s){
  FXuint rshift=0,gshift=0,bshift=0,rmul=0,gmul=0,bmul=0,rs=0,gs=0,bs=0;
  FXuint x,y,z,offset,pix,t;
  FXuchar *temp=dds.data;
  FXTRACE((150,"dds_decompress_RGBA\n"));
  if(rmask){ getShifts(rmask,rshift,rmul,rs); }
  if(gmask){ getShifts(gmask,gshift,gmul,gs); }
  if(bmask){ getShifts(bmask,bshift,bmul,bs); }
  FXTRACE((150,"rmask=0x%08x rshift=%2d rmul=%3d rs=%3d\n",rmask,rshift,rmul,rs));
  FXTRACE((150,"gmask=0x%08x gshift=%2d gmul=%3d gs=%3d\n",gmask,gshift,gmul,gs));
  FXTRACE((150,"bmask=0x%08x bshift=%2d bmul=%3d bs=%3d\n",bmask,bshift,bmul,bs));
  for(z=offset=0; z<dds.header.dwDepth; ++z){
    for(y=0; y<dds.header.dwHeight; ++y){
      for(x=0; x<dds.header.dwWidth; ++x){
        pix=(((FXuint)temp[3])<<24)|(((FXuint)temp[2])<<16)|(((FXuint)temp[1])<<8)|((FXuint)temp[0]);
        t=(pix&bmask)>>bshift; image[offset+0]=(t*bmul)>>bs;
        t=(pix&gmask)>>gshift; image[offset+1]=(t*gmul)>>gs;
        t=(pix&rmask)>>rshift; image[offset+2]=(t*rmul)>>rs;
        image[offset+3]=255;
        offset+=4;
        temp+=s;
        }
      }
    }
  return true;
  }


// General decompress integer pixel with alpha
static FXbool dds_decompress_RGBA(const DDSImage& dds,FXuchar *image,FXuint bmask,FXuint gmask,FXuint rmask,FXuint amask,FXuint s){
  FXuint rshift=0,gshift=0,bshift=0,ashift=0,rmul=0,gmul=0,bmul=0,amul=0,rs=0,gs=0,bs=0,as=0;
  FXuint x,y,z,offset,pix,t;
  FXuchar *temp=dds.data;
  FXTRACE((150,"dds_decompress_RGBA\n"));
  if(rmask){ getShifts(rmask,rshift,rmul,rs); }
  if(gmask){ getShifts(gmask,gshift,gmul,gs); }
  if(bmask){ getShifts(bmask,bshift,bmul,bs); }
  if(amask){ getShifts(amask,ashift,amul,as); }
  FXTRACE((150,"rmask=0x%08x rshift=%2d rmul=%3d rs=%3d\n",rmask,rshift,rmul,rs));
  FXTRACE((150,"gmask=0x%08x gshift=%2d gmul=%3d gs=%3d\n",gmask,gshift,gmul,gs));
  FXTRACE((150,"bmask=0x%08x bshift=%2d bmul=%3d bs=%3d\n",bmask,bshift,bmul,bs));
  FXTRACE((150,"amask=0x%08x ashift=%2d amul=%3d as=%3d\n",amask,ashift,amul,as));
  for(z=offset=0; z<dds.header.dwDepth; ++z){
    for(y=0; y<dds.header.dwHeight; ++y){
      for(x=0; x<dds.header.dwWidth; ++x){
        pix=(((FXuint)temp[3])<<24)|(((FXuint)temp[2])<<16)|(((FXuint)temp[1])<<8)|((FXuint)temp[0]);
        t=(pix&bmask)>>bshift; image[offset+0]=(t*bmul)>>bs;
        t=(pix&gmask)>>gshift; image[offset+1]=(t*gmul)>>gs;
        t=(pix&rmask)>>rshift; image[offset+2]=(t*rmul)>>rs;
        t=(pix&amask)>>ashift; image[offset+3]=(t*amul)>>as;
        offset+=4;
        temp+=s;
        }
      }
    }
  return true;
  }


// Decompress Luminance
static FXbool dds_decompress_LUM(const DDSImage& dds,FXuchar *image,FXuint cmask,FXuint s){
  FXuint cshift=0,cmul=0,cs=0;
  FXuint x,y,z,offset,pix,t;
  FXuchar *temp=dds.data;
  FXTRACE((150,"dds_decompress_LUM\n"));
  if(cmask){ getShifts(cmask,cshift,cmul,cs); }
  FXTRACE((150,"cmask=0x%08x cshift=%2d cmul=%3d cs=%3d\n",cmask,cshift,cmul,cs));
  for(z=offset=0; z<dds.header.dwDepth; ++z){
    for(y=0; y<dds.header.dwHeight; ++y){
      for(x=0; x<dds.header.dwWidth; ++x){
        pix=(((FXuint)temp[3])<<24)|(((FXuint)temp[2])<<16)|(((FXuint)temp[1])<<8)|((FXuint)temp[0]);
        t=(pix&cmask)>>cshift; image[offset+0]=image[offset+1]=image[offset+2]=(t*cmul)>>cs; image[offset+3]=255;
        offset+=4;
        temp+=s;
        }
      }
    }
  return true;
  }


// Decompress Luminance and alpha
static FXbool dds_decompress_LUMA(const DDSImage& dds,FXuchar *image,FXuint cmask,FXuint amask,FXuint s){
  FXuint cshift=0,ashift=0,cmul=0,amul=0,cs=0,as=0;
  FXuint x,y,z,offset,pix,t;
  FXuchar *temp=dds.data;
  FXTRACE((150,"dds_decompress_LUMA\n"));
  if(cmask){ getShifts(cmask,cshift,cmul,cs); }
  if(amask){ getShifts(amask,ashift,amul,as); }
  FXTRACE((150,"cmask=0x%08x cshift=%2d cmul=%3d cs=%3d\n",cmask,cshift,cmul,cs));
  FXTRACE((150,"amask=0x%08x ashift=%2d amul=%3d as=%3d\n",amask,ashift,amul,as));
  for(z=offset=0; z<dds.header.dwDepth; ++z){
    for(y=0; y<dds.header.dwHeight; ++y){
      for(x=0; x<dds.header.dwWidth; ++x){
        pix=(((FXuint)temp[3])<<24)|(((FXuint)temp[2])<<16)|(((FXuint)temp[1])<<8)|((FXuint)temp[0]);
        t=(pix&cmask)>>cshift; image[offset+0]=image[offset+1]=image[offset+2]=(t*cmul)>>cs;
        t=(pix&amask)>>ashift; image[offset+3]=(t*amul)>>as;
        offset+=4;
        temp+=s;
        }
      }
    }
  return true;
  }


// Decompress R16F
static FXbool dds_decompress_R16F(const DDSImage& dds,FXuchar *image){
  FXuint count=dds.header.dwDepth*dds.header.dwHeight*dds.header.dwWidth*4;
  FXhalf *temp=(FXhalf*)dds.data;
  FXuint p=0;
  FXTRACE((150,"dds_decompress_R16F\n"));
  while(p<count){
    image[p+0]=0;
    image[p+1]=0;
    image[p+2]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    image[p+3]=255;
    p+=4;
    }
  return true;
  }


// Decompress G16R16F
static FXbool dds_decompress_G16R16F(const DDSImage& dds,FXuchar *image){
  FXuint count=dds.header.dwDepth*dds.header.dwHeight*dds.header.dwWidth*4;
  FXhalf *temp=(FXhalf*)dds.data;
  FXuint p=0;
  FXTRACE((150,"dds_decompress_G16R16F\n"));
  while(p<count){
    image[p+0]=0;
    image[p+2]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    image[p+1]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    image[p+3]=255;
    p+=4;
    }
  return true;
  }


// Decompress A16B16G16R16F
static FXbool dds_decompress_A16B16G16R16F(const DDSImage& dds,FXuchar *image){
  FXuint count=dds.header.dwDepth*dds.header.dwHeight*dds.header.dwWidth*4;
  FXhalf *temp=(FXhalf*)dds.data;
  FXuint p=0;
  FXTRACE((150,"dds_decompress_A16B16G16R16F\n"));
  while(p<count){
    image[p+2]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    image[p+1]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    image[p+0]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    image[p+3]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    p+=4;
    }
  return true;
  }


// Decompress A16B16G16R16
static FXbool dds_decompress_A16B16G16R16(const DDSImage& dds,FXuchar *image){
  FXuint count=dds.header.dwDepth*dds.header.dwHeight*dds.header.dwWidth*4;
  FXushort *temp=(FXushort*)dds.data;
  FXuint p=0;
  FXTRACE((150,"dds_decompress_A16B16G16R16\n"));
  while(p<count){
    image[p+2]=(FXuchar)(*temp++ / 257);
    image[p+1]=(FXuchar)(*temp++ / 257);
    image[p+0]=(FXuchar)(*temp++ / 257);
    image[p+3]=(FXuchar)(*temp++ / 257);
    p+=4;
    }
  return true;
  }


// Decompress R32F
static FXbool dds_decompress_R32F(const DDSImage& dds,FXuchar *image){
  FXuint count=dds.header.dwDepth*dds.header.dwHeight*dds.header.dwWidth*4;
  FXfloat *temp=(FXfloat*)dds.data;
  FXuint p=0;
  FXTRACE((150,"dds_decompress_R32F\n"));
  while(p<count){
    image[p+0]=0;
    image[p+1]=0;
    image[p+2]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    image[p+3]=255;
    p+=4;
    }
  return true;
  }


// Decompress G32R32F
static FXbool dds_decompress_G32R32F(const DDSImage& dds,FXuchar *image){
  FXuint count=dds.header.dwDepth*dds.header.dwHeight*dds.header.dwWidth*4;
  FXfloat *temp=(FXfloat*)dds.data;
  FXuint p=0;
  FXTRACE((150,"dds_decompress_G32R32F\n"));
  while(p<count){
    image[p+2]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    image[p+1]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    image[p+0]=0;
    image[p+3]=255;
    p+=4;
    }
  return true;
  }


// Decompress A32B32G32R32F
static FXbool dds_decompress_A32B32G32R32F(const DDSImage& dds,FXuchar *image){
  FXuint count=dds.header.dwDepth*dds.header.dwHeight*dds.header.dwWidth*4;
  FXfloat *temp=(FXfloat*)dds.data;
  FXuint p=0;
  FXTRACE((150,"dds_decompress_A32B32G32R32F\n"));
  while(p<count){
    image[p+2]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    image[p+1]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    image[p+0]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    image[p+3]=(FXuchar)(*temp++ * 255.0f + 0.5f);
    p+=4;
    }
  return true;
  }


// Decompress R8G8_B8G8
static FXbool dds_decompress_RGBG(const DDSImage& dds,FXuchar *image){
  FXuint x,y,z,offset;
  FXuchar *temp=dds.data;
  FXTRACE((150,"dds_decompress_RGBG\n"));
  for(z=offset=0; z<dds.header.dwDepth; z+=1){
    for(y=0; y<dds.header.dwHeight; y+=1){
      for(x=0; x<dds.header.dwWidth; x+=2){
        image[offset+0]=temp[3];
        image[offset+1]=temp[0];
        image[offset+2]=temp[1];
        image[offset+3]=255;
        image[offset+4]=temp[3];
        image[offset+5]=temp[2];
        image[offset+6]=temp[1];
        image[offset+7]=255;
        offset+=8;
        temp+=4;
        }
      }
    }
  return true;
  }


// Decompress G8R8_G8B8
static FXbool dds_decompress_GRGB(const DDSImage& dds,FXuchar *image){
  FXuint x,y,z,offset;
  FXuchar *temp=dds.data;
  FXTRACE((150,"dds_decompress_GRGB\n"));
  for(z=offset=0; z<dds.header.dwDepth; z+=1){
    for(y=0; y<dds.header.dwHeight; y+=1){
      for(x=0; x<dds.header.dwWidth; x+=2){
        image[offset+0]=temp[2];
        image[offset+1]=temp[1];
        image[offset+2]=temp[0];
        image[offset+3]=255;
        image[offset+4]=temp[2];
        image[offset+5]=temp[3];
        image[offset+6]=temp[0];
        image[offset+7]=255;
        offset+=8;
        temp+=4;
        }
      }
    }
  return true;
  }


// Check if stream contains a BMP
FXbool fxcheckDDS(FXStream& store){
  FXuchar signature[4];
  store.load(signature,4);
  store.position(-4,FXFromCurrent);
  return signature[0]=='D' && signature[1]=='D' && signature[2]=='S' && signature[3]==' ';
  }


// Load image from stream
FXbool fxloadDDS(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& depth){
  FXbool swap=store.swapBytes();
  FXbool ok=false;
  DDSImage dds;

  // Null out
  data=nullptr;
  width=0;
  height=0;
  depth=0;

  // Bitmaps are little-endian
  store.setBigEndian(false);

  // Check header
  store >> dds.magic;
  if(dds.magic==DDSD_MAGIC){
    store >> dds.header.dwSize;
    if(dds.header.dwSize!=124) goto x;                  // Unexpected size; bail!
    store >> dds.header.dwFlags;
    store >> dds.header.dwHeight;
    store >> dds.header.dwWidth;
    store >> dds.header.dwLinearSize;
    store >> dds.header.dwDepth;
    store >> dds.header.dwMipMapCount;
    store.load(dds.header.dwReserved1,11);
    store >> dds.header.ddpf.dwSize;
    store >> dds.header.ddpf.dwFlags;
    store >> dds.header.ddpf.dwFourCC;
    store >> dds.header.ddpf.dwRGBBitCount;
    store >> dds.header.ddpf.dwRBitMask;
    store >> dds.header.ddpf.dwGBitMask;
    store >> dds.header.ddpf.dwBBitMask;
    store >> dds.header.ddpf.dwABitMask;
    store >> dds.header.dwCaps;
    store >> dds.header.dwCaps2;
    store >> dds.header.dwCaps3;
    store >> dds.header.dwCaps4;
    store >> dds.header.dwReserved2;

    // Load DX10 Header if present
    if(dds.header.ddpf.dwFourCC==D3DFMT_DX10){          // Parse over DX10 header
      store >> dds.xheader.dxgiFormat;
      store >> dds.xheader.resourceDimension;
      store >> dds.xheader.miscFlag;
      store >> dds.xheader.arraySize;
      store >> dds.xheader.reserved;
      }
    else{
      dds.xheader.dxgiFormat=DXGI_FORMAT_UNKNOWN;
      dds.xheader.resourceDimension=D3D10_RESOURCE_DIMENSION_UNKNOWN;
      dds.xheader.miscFlag=0;
      dds.xheader.arraySize=1;
      dds.xheader.reserved=0;
      }

    FXTRACE((150,"dds.magic=0x%08x\n",dds.magic));
    FXTRACE((150,"dds.header.dwSize=%d\n",dds.header.dwSize));
    FXTRACE((150,"dds.header.dwFlags=0x%08x: %s%s%s%s%s%s%s%s\n",dds.header.dwFlags,(dds.header.dwFlags&DDSD_CAPS)?"DDSD_CAPS ":"",(dds.header.dwFlags&DDSD_HEIGHT)?"DDSD_HEIGHT ":"",(dds.header.dwFlags&DDSD_WIDTH)?"DDSD_WIDTH ":"",(dds.header.dwFlags&DDSD_PITCH)?"DDSD_PITCH ":"",(dds.header.dwFlags&DDSD_PIXELFORMAT)?"DDSD_PIXELFORMAT ":"",(dds.header.dwFlags&DDSD_MIPMAPCOUNT)?"DDSD_MIPMAPCOUNT ":"",(dds.header.dwFlags&DDSD_LINEARSIZE)?"DDSD_LINEARSIZE ":"",(dds.header.dwFlags&DDSD_DEPTH)?"DDSD_DEPTH":""));
    FXTRACE((150,"dds.header.dwHeight=%d\n",dds.header.dwHeight));
    FXTRACE((150,"dds.header.dwWidth=%d\n",dds.header.dwWidth));
    FXTRACE((150,"dds.header.dwDepth=%d\n",dds.header.dwDepth));
    FXTRACE((150,"dds.header.dwLinearSize=%d\n",dds.header.dwLinearSize));
    FXTRACE((150,"dds.header.dwMipMapCount=%d\n",dds.header.dwMipMapCount));
    FXTRACE((150,"dds.header.ddpf.dwSize=%d\n",dds.header.ddpf.dwSize));
    FXTRACE((150,"dds.header.ddpf.dwFlags=0x%08x: %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",dds.header.ddpf.dwFlags,(dds.header.ddpf.dwFlags&DDPF_ALPHAPIXELS)?"DDPF_ALPHAPIXELS ":"",(dds.header.ddpf.dwFlags&DDPF_ALPHA)?"DDPF_ALPHA ":"",(dds.header.ddpf.dwFlags&DDPF_FOURCC)?"DDPF_FOURCC ":"",(dds.header.ddpf.dwFlags&DDPF_PALETTEINDEXED4)?"DDPF_PALETTEINDEXED4 ":"",(dds.header.ddpf.dwFlags&DDPF_PALETTEINDEXED8)?"DDPF_PALETTEINDEXED8 ":"",(dds.header.ddpf.dwFlags&DDPF_RGB)?"DDPF_RGB ":"",(dds.header.ddpf.dwFlags&DDPF_COMPRESSED)?"DDPF_COMPRESSED ":"",(dds.header.ddpf.dwFlags&DDPF_RGBTOYUV)?"DDPF_RGBTOYUV ":"",(dds.header.ddpf.dwFlags&DDPF_YUV)?"DDPF_YUV ":"",(dds.header.ddpf.dwFlags&DDPF_ZBUFFER)?"DDPF_ZBUFFER ":"",(dds.header.ddpf.dwFlags&DDPF_PALETTEINDEXED1)?"DDPF_PALETTEINDEXED1 ":"",(dds.header.ddpf.dwFlags&DDPF_PALETTEINDEXED2)?"DDPF_PALETTEINDEXED2 ":"",(dds.header.ddpf.dwFlags&DDPF_ZPIXELS)?"DDPF_ZPIXELS ":"",(dds.header.ddpf.dwFlags&DDPF_STENCILBUFFER)?"DDPF_STENCILBUFFER ":"",(dds.header.ddpf.dwFlags&DDPF_ALPHAPREMULT)?"DDPF_ALPHAPREMULT ":"",(dds.header.ddpf.dwFlags&DDPF_LUMINANCE)?"DDPF_LUMINANCE ":"",(dds.header.ddpf.dwFlags&DDPF_BUMPLUMINANCE)?"DDPF_BUMPLUMINANCE ":"",(dds.header.ddpf.dwFlags&DDPF_NORMAL)?"DDPF_NORMAL":""));
    FXTRACE((150,"dds.header.ddpf.dwFourCC=0x%08x (%d) (%c%c%c%c)\n",dds.header.ddpf.dwFourCC,dds.header.ddpf.dwFourCC,dds.header.ddpf.dwFourCC&255,(dds.header.ddpf.dwFourCC>>8)&255,(dds.header.ddpf.dwFourCC>>16)&255,(dds.header.ddpf.dwFourCC>>24)&255));
    FXTRACE((150,"dds.header.ddpf.dwRGBBitCount=%d\n",dds.header.ddpf.dwRGBBitCount));
    FXTRACE((150,"dds.header.ddpf.dwRBitMask=0x%08x\n",dds.header.ddpf.dwRBitMask));
    FXTRACE((150,"dds.header.ddpf.dwGBitMask=0x%08x\n",dds.header.ddpf.dwGBitMask));
    FXTRACE((150,"dds.header.ddpf.dwBBitMask=0x%08x\n",dds.header.ddpf.dwBBitMask));
    FXTRACE((150,"dds.header.ddpf.dwABitMask=0x%08x\n",dds.header.ddpf.dwABitMask));

    FXTRACE((150,"dds.header.dwCaps =0x%08x: %s%s%s\n",dds.header.dwCaps,(dds.header.dwCaps&DDSCAPS_COMPLEX)?"DDSCAPS_COMPLEX ":"",(dds.header.dwCaps&DDSCAPS_TEXTURE)?"DDSCAPS_TEXTURE ":"",(dds.header.dwCaps&DDSCAPS_MIPMAP)?"DDSCAPS_MIPMAP":""));
    FXTRACE((150,"dds.header.dwCaps2=0x%08x: %s%s%s%s%s%s%s%s\n",dds.header.dwCaps2,(dds.header.dwCaps2&DDSCAPS2_CUBEMAP)?"DDSCAPS2_CUBEMAP ":"",(dds.header.dwCaps2&DDSCAPS2_VOLUME)?"DDSCAPS2_VOLUME ":"",(dds.header.dwCaps2&DDSCAPS2_CUBEMAP_POSITIVEX)?"DDSCAPS2_CUBEMAP_POSITIVEX ":"",(dds.header.dwCaps2&DDSCAPS2_CUBEMAP_NEGATIVEX)?"DDSCAPS2_CUBEMAP_NEGATIVEX ":"",(dds.header.dwCaps2&DDSCAPS2_CUBEMAP_POSITIVEY)?"DDSCAPS2_CUBEMAP_POSITIVEY ":"",(dds.header.dwCaps2&DDSCAPS2_CUBEMAP_NEGATIVEY)?"DDSCAPS2_CUBEMAP_NEGATIVEY ":"",(dds.header.dwCaps2&DDSCAPS2_CUBEMAP_POSITIVEZ)?"DDSCAPS2_CUBEMAP_POSITIVEZ ":"",(dds.header.dwCaps2&DDSCAPS2_CUBEMAP_NEGATIVEZ)?"DDSCAPS2_CUBEMAP_NEGATIVEZ ":""));
    FXTRACE((150,"dds.header.dwCaps3=0x%08x\n",dds.header.dwCaps3));
    FXTRACE((150,"dds.header.dwCaps4=0x%08x\n",dds.header.dwCaps4));

    FXTRACE((150,"dds.xheader.dxgiFormat=%d\n",dds.xheader.dxgiFormat));
    FXTRACE((150,"dds.xheader.resourceDimension=%d\n",dds.xheader.resourceDimension));
    FXTRACE((150,"dds.xheader.miscFlag=%d\n",dds.xheader.miscFlag));
    FXTRACE((150,"dds.xheader.arraySize=%d\n",dds.xheader.arraySize));

    // Fix depth
    if(!(dds.header.dwFlags&DDSD_DEPTH) || (dds.header.dwDepth==0)) dds.header.dwDepth=1;

    // Fix mipmap count
    if(!(dds.header.dwFlags&DDSD_MIPMAPCOUNT) || (dds.header.dwMipMapCount==0)) dds.header.dwMipMapCount=1;

    // Set image size to return
    width=dds.header.dwWidth;
    height=dds.header.dwHeight;
    depth=dds.header.dwDepth;

    // Perhaps broken format; assume DDPF_FOURCC
    if(dds.header.ddpf.dwFlags==0 && dds.header.ddpf.dwFourCC!=0){
      dds.header.ddpf.dwFlags=DDPF_FOURCC;
      }

    // Figure out how much to allocate for compressed data
    if(dds.header.ddpf.dwFlags&DDPF_FOURCC){
      switch(dds.header.ddpf.dwFourCC){
        case D3DFMT_DXT1:
        case D3DFMT_ATI1:
          dds.size=((width+3)>>2)*((height+3)>>2)*depth*8;
          break;
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
        case D3DFMT_ATI2:
        case D3DFMT_RXGB:
          dds.size=((width+3)>>2)*((height+3)>>2)*depth*16;
          break;
        case D3DFMT_A1:
          dds.size=((width+7)>>3)*height*depth;
          break;
        case D3DFMT_A2XY:
        case D3DFMT_UYVY:
        case D3DFMT_YUY2:
          goto x;       // Unsupported compression code
        case D3DFMT_R16F:
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_A8R3G3B2:
        case D3DFMT_X4R4G4B4:
        case D3DFMT_R8G8_B8G8:
        case D3DFMT_G8R8_G8B8:
        case D3DFMT_L16:
        case D3DFMT_A8P8:
        case D3DFMT_A8L8:
          dds.size=width*height*depth*2;
          break;
        case D3DFMT_G16R16F:
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_A8B8G8R8:
        case D3DFMT_X8B8G8R8:
        case D3DFMT_G16R16:
        case D3DFMT_A2R10G10B10:
        case D3DFMT_R32F:
        case D3DFMT_A2B10G10R10:
        case D3DFMT_A2B10G10R10_XR_BIAS:
          dds.size=width*height*depth*4;
          break;
        case D3DFMT_A32B32G32R32F:
          dds.size=width*height*depth*16;
          break;
        case D3DFMT_R8G8B8:
          dds.size=width*height*depth*3;
          break;
        case D3DFMT_R3G3B2:
        case D3DFMT_A8:
        case D3DFMT_P8:
        case D3DFMT_L8:
        case D3DFMT_A4L4:
          dds.size=width*height*depth;
          break;
        case D3DFMT_A16B16G16R16F:
        case D3DFMT_G32R32F:
        case D3DFMT_A16B16G16R16:
          dds.size=width*height*depth*8;
          break;
        default:
          goto x;       // Unsupported compression code
        }
      }

    // Figure out how much to allocate for RGB
    else if(dds.header.ddpf.dwFlags&DDPF_RGB){
      dds.size=width*height*depth*dds.header.ddpf.dwRGBBitCount/8;
      }

    // Luminance
    else if(dds.header.ddpf.dwFlags&DDPF_LUMINANCE){
      dds.size=width*height*depth*dds.header.ddpf.dwRGBBitCount/8;
      }

    // Unsupported format
    else{
      goto x;           // Not supported
      }

    FXTRACE((150,"dds.size=%d\n",dds.size));

    // Allocate array for compressed data
    if(allocElms(dds.data,dds.size)){

      // Allocate output image
      if(allocElms(data,width*height*depth)){

        // Load temp array
        store.load(dds.data,dds.size);

        // FOURCC format
        if(dds.header.ddpf.dwFlags&DDPF_FOURCC){
          switch(dds.header.ddpf.dwFourCC){
            case D3DFMT_DXT1:
              ok=dds_decompress_DXT1(dds,(FXuchar*)data);
              break;
            case D3DFMT_DXT2:
              ok=dds_decompress_DXT2(dds,(FXuchar*)data);
              break;
            case D3DFMT_DXT3:
              ok=dds_decompress_DXT3(dds,(FXuchar*)data);
              break;
            case D3DFMT_DXT4:
              ok=dds_decompress_DXT4(dds,(FXuchar*)data);
              break;
            case D3DFMT_DXT5:
              ok=dds_decompress_DXT5(dds,(FXuchar*)data);
              break;
            case D3DFMT_ATI1:
              ok=dds_decompress_BC4(dds,(FXuchar*)data);
              break;
            case D3DFMT_ATI2:
              ok=dds_decompress_3DC(dds,(FXuchar*)data);
              break;
            case D3DFMT_RXGB:
              ok=dds_decompress_RXGB(dds,(FXuchar*)data);
              break;
            case D3DFMT_A2XY:
            case D3DFMT_UYVY:
            case D3DFMT_YUY2:
              break;
            case D3DFMT_R8G8_B8G8:
              ok=dds_decompress_RGBG(dds,(FXuchar*)data);
              break;
            case D3DFMT_G8R8_G8B8:
              ok=dds_decompress_GRGB(dds,(FXuchar*)data);
              break;
            case D3DFMT_R16F:
              ok=dds_decompress_R16F(dds,(FXuchar*)data);
              break;
            case D3DFMT_G16R16F:
              ok=dds_decompress_G16R16F(dds,(FXuchar*)data);
              break;
            case D3DFMT_A16B16G16R16F:
              ok=dds_decompress_A16B16G16R16F(dds,(FXuchar*)data);
              break;
            case D3DFMT_R32F:
              ok=dds_decompress_R32F(dds,(FXuchar*)data);
              break;
            case D3DFMT_G32R32F:
              ok=dds_decompress_G32R32F(dds,(FXuchar*)data);
              break;
            case D3DFMT_A32B32G32R32F:
              ok=dds_decompress_A32B32G32R32F(dds,(FXuchar*)data);
              break;
            case D3DFMT_R8G8B8:
              ok=dds_decompress_RGB(dds,(FXuchar*)data,0x0000FF,0x00FF00,0xFF0000,3);
              break;
            case D3DFMT_A8R8G8B8:
              ok=dds_decompress_RGBA(dds,(FXuchar*)data,0x000000FF,0x0000FF00,0x00FF0000,0xFF000000,4);
              break;
            case D3DFMT_X8R8G8B8:
              ok=dds_decompress_RGB(dds,(FXuchar*)data,0x0000FF,0x00FF00,0xFF0000,4);
              break;
            case D3DFMT_R5G6B5:
              ok=dds_decompress_RGB(dds,(FXuchar*)data,0x001F,0x07E0,0xF800,2);
              break;
            case D3DFMT_X1R5G5B5:
              ok=dds_decompress_RGB(dds,(FXuchar*)data,0x001F,0x03E0,0x7C00,2);
              break;
            case D3DFMT_A1R5G5B5:
              ok=dds_decompress_RGBA(dds,(FXuchar*)data,0x001F,0x03E0,0x7C00,0x8000,2);
              break;
            case D3DFMT_A4R4G4B4:
              ok=dds_decompress_RGBA(dds,(FXuchar*)data,0x000F,0x00F0,0x0F00,0xF000,2);
              break;
            case D3DFMT_R3G3B2:
              ok=dds_decompress_RGB(dds,(FXuchar*)data,0x07,0x38,0xC0,1);
              break;
            case D3DFMT_A8:
              ok=dds_decompress_RGBA(dds,(FXuchar*)data,0x00,0x00,0x00,0xFF,1);
              break;
            case D3DFMT_A8R3G3B2:
              ok=dds_decompress_RGBA(dds,(FXuchar*)data,0x0003,0x001C,0x00E0,0xFF00,2);
              break;
            case D3DFMT_X4R4G4B4:
              ok=dds_decompress_RGB(dds,(FXuchar*)data,0x000F,0x00F0,0x0F00,2);
              break;
            case D3DFMT_A8B8G8R8:
              ok=dds_decompress_RGBA(dds,(FXuchar*)data,0x00FF0000,0x0000FF00,0x000000FF,0xFF000000,4);
              break;
            case D3DFMT_X8B8G8R8:
              ok=dds_decompress_RGB(dds,(FXuchar*)data,0x00FF0000,0x0000FF00,0x000000FF,4);
              break;
            case D3DFMT_G16R16:
              ok=dds_decompress_RGB(dds,(FXuchar*)data,0x00000000,0xFFFF0000,0x0000FFFF,4);
              break;
            case D3DFMT_A2R10G10B10:
              ok=dds_decompress_RGBA(dds,(FXuchar*)data,0x000003FF,0x000FFC00,0x3FF00000,0xC0000000,2);
              break;
            case D3DFMT_A16B16G16R16:
              ok=dds_decompress_A16B16G16R16(dds,(FXuchar*)data);
              break;
            case D3DFMT_P8:
            case D3DFMT_L8:
              ok=dds_decompress_LUM(dds,(FXuchar*)data,0xFF,1);
              break;
            case D3DFMT_A8P8:
            case D3DFMT_A8L8:
              ok=dds_decompress_LUMA(dds,(FXuchar*)data,0x00FF,0xFF00,2);
              break;
            case D3DFMT_A4L4:
              ok=dds_decompress_LUMA(dds,(FXuchar*)data,0x0F,0xF0,1);
              break;
            case D3DFMT_L16:
              ok=dds_decompress_LUM(dds,(FXuchar*)data,0xFFFF,2);
              break;
            case D3DFMT_A1:
              break;
            case D3DFMT_A2B10G10R10:
            case D3DFMT_A2B10G10R10_XR_BIAS:
              ok=dds_decompress_RGBA(dds,(FXuchar*)data,0x3FF00000,0x000FFC00,0x000003FF,0xC0000000,2);
              break;
            }
          }

        // RGB format
        else if(dds.header.ddpf.dwFlags&DDPF_RGB){
          if(dds.header.ddpf.dwFlags&DDPF_ALPHAPIXELS)
            ok=dds_decompress_RGBA(dds,(FXuchar*)data,dds.header.ddpf.dwBBitMask,dds.header.ddpf.dwGBitMask,dds.header.ddpf.dwRBitMask,dds.header.ddpf.dwABitMask,(dds.header.ddpf.dwRGBBitCount+7)>>3);
          else
            ok=dds_decompress_RGB(dds,(FXuchar*)data,dds.header.ddpf.dwBBitMask,dds.header.ddpf.dwGBitMask,dds.header.ddpf.dwRBitMask,(dds.header.ddpf.dwRGBBitCount+7)>>3);
          }

        // Lumimance format
        else if(dds.header.ddpf.dwFlags&DDPF_LUMINANCE){
          if(dds.header.ddpf.dwFlags&DDPF_ALPHAPIXELS)
            ok=dds_decompress_LUMA(dds,(FXuchar*)data,dds.header.ddpf.dwRBitMask,dds.header.ddpf.dwABitMask,(dds.header.ddpf.dwRGBBitCount+7)/8);
          else
            ok=dds_decompress_LUM(dds,(FXuchar*)data,dds.header.ddpf.dwRBitMask,(dds.header.ddpf.dwRGBBitCount+7)/8);
          }
        }

      // Free temp array of encoded pixels
      freeElms(dds.data);
      }
    }

  // Restore original byte orientation
x:store.swapBytes(swap);

  // Done
  return ok;
  }


// Save a dds file to a stream
FXbool fxsaveDDS(FXStream& store,FXColor* data,FXint width,FXint height,FXint depth){
  DDSImage dds;
  FXbool swap;

  // Must make sense
  if(!data || width<=0 || height<=0 || depth<=0) return false;

  // Switch byte order for the duration
  swap=store.swapBytes();
  store.setBigEndian(false);

  // Initialize header
  dds.magic=DDSD_MAGIC;
  dds.header.dwSize=sizeof(DDSHeader);
  dds.header.dwFlags=DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_MIPMAPCOUNT|DDSD_LINEARSIZE;
  dds.header.dwHeight=height;
  dds.header.dwWidth=width;
  dds.header.dwLinearSize=width*height*depth*4;
  dds.header.dwDepth=depth;
  dds.header.dwMipMapCount=1;
  memset(dds.header.dwReserved1,0,sizeof(dds.header.dwReserved1));
  dds.header.ddpf.dwSize=sizeof(DDSPixelFormat);
  dds.header.ddpf.dwFlags=DDPF_RGB;
  dds.header.ddpf.dwFourCC=0;
  dds.header.ddpf.dwRGBBitCount=32;
  dds.header.ddpf.dwBBitMask=0x000000ff;
  dds.header.ddpf.dwGBitMask=0x0000ff00;
  dds.header.ddpf.dwRBitMask=0x00ff0000;
  dds.header.ddpf.dwABitMask=0xff000000;
  if(1<depth){
    dds.header.dwCaps=DDSCAPS_COMPLEX|DDSCAPS_TEXTURE;
    dds.header.dwCaps2=DDSCAPS2_VOLUME;
    }
  else{
    dds.header.dwCaps=DDSCAPS_TEXTURE;
    dds.header.dwCaps2=0;
    }
  dds.header.dwCaps3=0;
  dds.header.dwCaps4=0;
  dds.header.dwReserved2=0;

  // Start saving now
  store << dds.magic;
  store << dds.header.dwSize;
  store << dds.header.dwFlags;
  store << dds.header.dwHeight;
  store << dds.header.dwWidth;
  store << dds.header.dwLinearSize;
  store << dds.header.dwDepth;
  store << dds.header.dwMipMapCount;
  store.save(dds.header.dwReserved1,11);
  store << dds.header.ddpf.dwSize;
  store << dds.header.ddpf.dwFlags;
  store << dds.header.ddpf.dwFourCC;
  store << dds.header.ddpf.dwRGBBitCount;
  store << dds.header.ddpf.dwRBitMask;
  store << dds.header.ddpf.dwGBitMask;
  store << dds.header.ddpf.dwBBitMask;
  store << dds.header.ddpf.dwABitMask;
  store << dds.header.dwCaps;
  store << dds.header.dwCaps2;
  store << dds.header.dwCaps3;
  store << dds.header.dwCaps4;
  store << dds.header.dwReserved2;

  // Data array
  store.save(data,width*height*depth);

  store.swapBytes(swap);
  return true;
  }

}

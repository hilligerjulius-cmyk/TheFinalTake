"""The Final Take palette (sRGB hex), mirrored from Core/FTVisuals.cpp (FTColors) and the
studio/city shades in FTStudioShell.cpp / FTCity.cpp. Assets bake these into vertex colours."""


def hex_rgb(h):
    """0xRRGGBB -> sRGB floats 0..1 (vertex colours are stored as sRGB bytes)."""
    return ((h >> 16 & 255) / 255.0, (h >> 8 & 255) / 255.0, (h & 255) / 255.0)


def lin_to_srgb(c):
    return c * 12.92 if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055


def srgb_to_lin(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def mix(a, b, t):
    return tuple(a[i] * (1 - t) + b[i] * t for i in range(3))


def shade(c, f):
    """Scale brightness in linear space (keeps hue), returns sRGB."""
    return tuple(min(1.0, lin_to_srgb(srgb_to_lin(x) * f)) for x in c)


# FTColors
TEAL = hex_rgb(0x1F9E96)
TEAL_DARK = hex_rgb(0x13706B)
TEAL_LIGHT = hex_rgb(0x5CCFC3)
CYAN = hex_rgb(0x33D6E8)
CORAL = hex_rgb(0xFF6F59)
CORAL_DARK = hex_rgb(0xD9483A)
YELLOW = hex_rgb(0xFFC93C)
AMBER = hex_rgb(0xFFA62B)
CREAM = hex_rgb(0xF6E7C8)
CREAM_DARK = hex_rgb(0xE0CBA0)
MAGENTA = hex_rgb(0xE0409A)
DEEP_BLUE = hex_rgb(0x2346A8)
BLUE = hex_rgb(0x3D7DD8)
SKY_BLUE = hex_rgb(0x7EC8F2)
NAVY = hex_rgb(0x1B2140)
NAVY_LIGHT = hex_rgb(0x2E3766)
RED = hex_rgb(0xE8322E)
WOOD = hex_rgb(0xB06A3B)
WOOD_DARK = hex_rgb(0x7A4526)
SAND = hex_rgb(0xF2C77B)
GREY = hex_rgb(0x8C93A6)
GREY_DARK = hex_rgb(0x4B5163)
CHARCOAL = hex_rgb(0x2B2F3A)
WHITE = hex_rgb(0xF7F7F2)
GREEN = hex_rgb(0x4DB35E)
GREEN_DARK = hex_rgb(0x2F7D45)
PURPLE = hex_rgb(0x7B5BD6)
ORANGE = hex_rgb(0xFF8A2B)
INK = hex_rgb(0x14131F)
SKIN_A = hex_rgb(0xF2C29B)
SKIN_B = hex_rgb(0xE3A77C)
SKIN_C = hex_rgb(0x8D5A3B)
SKIN_D = hex_rgb(0xF7D4BC)
HAIR_BROWN = hex_rgb(0x5A3522)
HAIR_DARK = hex_rgb(0x2A1B14)
HAIR_GINGER = hex_rgb(0xD9642B)
HAIR_BLACK = hex_rgb(0x1C1C22)

# studio shades (FTStudioShell.cpp)
ASPHALT = hex_rgb(0x3C4262)
SIDEWALK = hex_rgb(0x6E7391)
STAGE_FLOOR = hex_rgb(0x646B8C)
STAGE_WALL = hex_rgb(0x232A4F)
STAGE_WALL_LOW = hex_rgb(0x1C5C63)
LOBBY_WALL = hex_rgb(0xF3DFC0)
CEILING = hex_rgb(0xE2CFA8)
TAPE = hex_rgb(0xFFC93C)
TANK_BLUE = hex_rgb(0x3D7DD8)
BACKDROP = hex_rgb(0x7EC8F2)
CARPET = hex_rgb(0xC9304A)
TERRACOTTA = hex_rgb(0xD9784A)
BRASS = hex_rgb(0xE3B04B)
WARDROBE_WALL = hex_rgb(0xF2C6DD)
WINDOW_WARM = hex_rgb(0xFFD58A)
PUDDLE = hex_rgb(0x3456A8)
ROCK_A = hex_rgb(0x8E8AA3)
ROCK_B = hex_rgb(0xA29B8B)
BOOTH_PURPLE = hex_rgb(0x5B3F8C)
PROJ_WALL = hex_rgb(0x3B2E5E)
PROJ_CEIL = hex_rgb(0x2B2140)
PROJ_FLOOR = hex_rgb(0x4A3A2E)
STAIR_BLUE = hex_rgb(0x3A4570)
DECK_TEAL = hex_rgb(0x2B5F66)
WARDROBE_STAIR = hex_rgb(0x6B3F7A)
CORK = hex_rgb(0xC98B55)

# city shades (FTCity.cpp)
GROUND = hex_rgb(0x30355A)
WINDOW_DARK = hex_rgb(0x1B2140)
ALLEY_WALL = hex_rgb(0x262B4A)
LOT_GREY = hex_rgb(0x8C91AE)
PARK_GREEN = hex_rgb(0x3E8E5B)
HEDGE_GREEN = hex_rgb(0x2B6B47)
PLAZA_STONE = hex_rgb(0xCFC2A8)
PLAZA_JOINT = hex_rgb(0xAFA38B)
CINEMA_OUTER = hex_rgb(0x2B2F5E)
CINEMA_ROOF = hex_rgb(0x1E2140)
FOYER_WALL = hex_rgb(0x7A2E4A)
FOYER_CARPET = hex_rgb(0x8E2340)
HALL_WALL = hex_rgb(0x4A1E36)
HALL_FLOOR = hex_rgb(0x3A1A2C)
HALL_CEIL = hex_rgb(0x221430)
MEZZ_WOOD = hex_rgb(0x5A2A3E)
STAGE_WOOD = hex_rgb(0x5A3A2A)
CURTAIN_FOLD = hex_rgb(0x9C2038)
CINEMA_FLOOR = hex_rgb(0x3B2233)
BLD_NAVY_B = hex_rgb(0x3A3F6E)
BLD_NAVY_C = hex_rgb(0x2F355E)
BLD_PLUM = hex_rgb(0x4A3F72)
BLD_PLUM_B = hex_rgb(0x3E335E)
CHART_PANEL = hex_rgb(0x151A33)

# extra material tones for details (still on-palette)
CHROME = hex_rgb(0xC9D0DE)
RUBBER = hex_rgb(0x23252D)
GLASS = hex_rgb(0x9BD9F0)
LEAF_LIGHT = hex_rgb(0x6CC76F)
POPCORN = hex_rgb(0xFFE7A3)

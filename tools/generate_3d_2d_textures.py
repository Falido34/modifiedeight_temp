#!/usr/bin/env python3
import argparse
import json
import math
import os
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from PIL import Image, ImageDraw


REPO_ROOT = Path(__file__).resolve().parent.parent
NA_ASSETS = REPO_ROOT / "modifiedeight-newadditions" / "assets"

SEARCH_BLOCK_DIRS = [
    NA_ASSETS / "images" / "textures" / "blocks",
    NA_ASSETS / "textures" / "blocks",
    NA_ASSETS / "images" / "new_textures",
]
SEARCH_ITEM_DIRS = [
    NA_ASSETS / "images" / "textures" / "items",
    NA_ASSETS / "textures" / "items",
    NA_ASSETS / "images" / "item",
]
SEARCH_MOB_DIRS = [
    NA_ASSETS / "images" / "mob",
]

MOB_HEAD_SPECS = {
    "head_steve": {
        "skin": "char.png",
        "size": (64, 32),
        "face_uv": (8, 8, 16, 16),
        "hat_uv": (40, 8, 48, 16),
        "boxes": [
            ((-4, -4, -4), (8, 8, 8), (0, 0)),
            ((-4.5, -4.5, -4.5), (9, 9, 9), (32, 0), (8, 8, 8), 1),
        ],
    },
    "head_creeper": {
        "skin": "creeper.png",
        "size": (64, 32),
        "face_uv": (8, 8, 16, 16),
        "boxes": [((-4, -4, -4), (8, 8, 8), (0, 0))],
    },
    "head_zombie": {
        "skin": "zombie.png",
        "size": (64, 32),
        "face_uv": (8, 8, 16, 16),
        "boxes": [((-4, -4, -4), (8, 8, 8), (0, 0))],
    },
    "head_skeleton": {
        "skin": "skeleton.png",
        "size": (64, 32),
        "face_uv": (8, 8, 16, 16),
        "boxes": [((-4, -4, -4), (8, 8, 8), (0, 0))],
    },
    "head_spider": {
        "skin": "spider.png",
        "size": (64, 32),
        "face_uv": (40, 12, 48, 20),
        "boxes": [((-4, -4, -4), (8, 8, 8), (32, 4))],
    },
    "head_pigzombie": {
        "skin": "pigzombie.png",
        "size": (64, 32),
        "face_uv": (8, 8, 16, 16),
        "hat_uv": (40, 8, 48, 16),
        "boxes": [
            ((-4, -4, -4), (8, 8, 8), (0, 0)),
            ((-4.5, -4.5, -4.5), (9, 9, 9), (32, 0), (8, 8, 8), 1),
        ],
    },
    "head_slime": {
        "skin": "slime.png",
        "size": (64, 32),
        "face_uv": (8, 8, 16, 16),
        "boxes": [
            ((-3, -3, -3), (6, 6, 6), (0, 16)),
            ((-4, -4, -4), (8, 8, 8), (0, 0), None, 1),
        ],
    },
    "head_cow": {
        "skin": "cow.png",
        "size": (64, 32),
        "face_uv": (6, 6, 14, 14),
        "boxes": [
            ((-4, -4, -3), (8, 8, 6), (0, 0)),
            ((-5, -5, -1), (1, 3, 1), (22, 0)),
            ((4, -5, -1), (1, 3, 1), (22, 0)),
        ],
    },
    "head_pig": {
        "skin": "pig.png",
        "size": (64, 32),
        "face_uv": (8, 8, 16, 16),
        "boxes": [
            ((-4, -4, -4), (8, 8, 8), (0, 0)),
            ((-2, 0, -5), (4, 3, 1), (16, 16)),
        ],
    },
    "head_sheep": {
        "skin": "sheep_0.png",
        "size": (64, 64),
        "face_uv": (8, 8, 14, 14),
        "boxes": [((-3, -3, -4), (6, 6, 8), (0, 0))],
    },
    "head_chicken": {
        "skin": "chicken.png",
        "size": (64, 32),
        "face_uv": (3, 3, 7, 9),
        "boxes": [
            ((-2, -3, -1.5), (4, 6, 3), (0, 0)),
            ((-2, -1, -3.5), (4, 2, 2), (14, 0)),
            ((-1, 1, -2.5), (2, 2, 2), (14, 4)),
        ],
    },
    "head_villager": {
        "skin": "villager.png",
        "size": (64, 64),
        "face_uv": (8, 8, 16, 18),
        "boxes": [
            ((-4, -5, -4), (8, 10, 8), (0, 0)),
            ((-1, 2, -6), (2, 4, 2), (24, 0), None, 1),
        ],
    },
    "head_ocelot": {
        "skin": "ocelot.png",
        "size": (64, 32),
        "face_uv": (5, 5, 10, 9),
        "boxes": [
            ((-2.5, -2, -2.5), (5, 4, 5), (0, 0)),
            ((-1.5, 0, -3.5), (3, 2, 1), (1, 24)),
            ((-2, -3, -1), (1, 1, 2), (1, 17)),
            ((1, -3, -1), (1, 1, 2), (1, 17)),
        ],
    },
    "head_polarbear": {
        "skin": "polarbear.png",
        "size": (128, 64),
        "face_uv": (7, 7, 14, 14),
        "boxes": [
            ((-3.5, -3.5, -3.5), (7, 7, 7), (0, 0)),
            ((-2.5, 0.5, -6.5), (5, 3, 3), (0, 44)),
            ((-4.5, -4.5, -1.5), (2, 2, 1), (26, 0)),
            ((2.5, -4.5, -1.5), (2, 2, 1), (26, 0)),
        ],
    },
    "head_turtle": {
        "skin": "big_sea_turtle.png",
        "size": (128, 64),
        "face_uv": (9, 6, 15, 11),
        "boxes": [((-3, -2.5, -3), (6, 5, 6), (3, 0))],
    },
    "head_wolf": {
        "skin": "wolf.png",
        "size": (64, 32),
        "face_uv": (4, 4, 10, 10),
        "boxes": [
            ((-3, -3, -2), (6, 6, 4), (0, 0)),
            ((-3, -5, 0), (2, 2, 1), (16, 14)),
            ((1, -5, 0), (2, 2, 1), (16, 14)),
            ((-1.5, 0, -5), (3, 3, 4), (0, 10)),
        ],
    },
    "head_fox": {
        "skin": "fox.png",
        "size": (48, 32),
        "face_uv": (7, 11, 15, 17),
        "aspect": (1.0, 0.75),
        "boxes": [
            ((-4, -3, -3), (8, 6, 6), (1, 5)),
            ((-4, -5, -2), (2, 2, 1), (8, 1)),
            ((2, -5, -2), (2, 2, 1), (15, 1)),
            ((-2, 1, -6), (4, 2, 3), (6, 18)),
        ],
    },
}

GREEN_TINT = (124, 189, 107)

CATALOG: Dict[str, dict] = {
    "glowstone_ore": {"kind": "block", "shape": "cube", "faces": {"all": "glowstone_ore"}, "item_icon": "glowstone_ore"},
    "copper_ore": {"kind": "block", "shape": "cube", "faces": {"all": "copper_ore"}},
    "copper_block": {"kind": "block", "shape": "cube", "faces": {"all": "copper_block"}},
    "copper_bulb": {"kind": "block", "shape": "cube", "faces": {"all": "copper_bulb"}},
    "copper_bulb_lit": {"kind": "block", "shape": "cube", "faces": {"all": "copper_bulb_lit"}},
    "copper_bulb_powered": {"kind": "block", "shape": "cube", "faces": {"all": "copper_bulb_powered"}},
    "copper_bulb_lit_powered": {"kind": "block", "shape": "cube", "faces": {"all": "copper_bulb_lit_powered"}},
    "copper_grate": {"kind": "block", "shape": "cube", "faces": {"all": "copper_grate"}},
    "copper_slab": {"kind": "block", "shape": "slab", "faces": {"all": "copper_block"}},
    "copper_stairs": {"kind": "block", "shape": "stairs", "faces": {"all": "copper_block"}},
    "copper_trapdoor": {"kind": "block", "shape": "plate", "faces": {"all": "copper_trapdoor"}},
    "copper_bars": {"kind": "block", "shape": "pane", "faces": {"all": "copper_bars"}},
    "iron_bars": {"kind": "block", "shape": "pane", "faces": {"all": "iron_bars"}},
    "copper_door": {"kind": "block", "shape": "door", "faces": {"top": "copper_door_top", "bottom": "copper_door_bottom"}, "item_icon": "copper_door"},
    "jungle_door": {"kind": "block", "shape": "door", "faces": {"top": "jungle_door_top", "bottom": "jungle_door_bottom"}, "item_icon": "jungle_door"},
    "door_wood": {"kind": "block", "shape": "door", "faces": {"top": "door_wood_upper", "bottom": "door_wood_lower"}, "item_icon": "door_wood"},
    "door_spruce": {"kind": "block", "shape": "door", "faces": {"top": "door_spruce_upper", "bottom": "door_spruce_lower"}, "item_icon": "door_spruce"},
    "door_birch": {"kind": "block", "shape": "door", "faces": {"top": "door_birch_upper", "bottom": "door_birch_lower"}, "item_icon": "door_birch"},
    "iron_trapdoor": {"kind": "block", "shape": "plate", "faces": {"all": "iron_trapdoor"}},
    "jungle_trapdoor": {"kind": "block", "shape": "plate", "faces": {"all": "jungle_trapdoor"}},
    "trapdoor": {"kind": "block", "shape": "plate", "faces": {"all": "trapdoor"}},
    "trapdoor_spruce": {"kind": "block", "shape": "plate", "faces": {"all": "trapdoor_spruce"}},
    "trapdoor_birch": {"kind": "block", "shape": "plate", "faces": {"all": "trapdoor_birch"}},
    "trapdoor_jungle": {"kind": "block", "shape": "plate", "faces": {"all": "trapdoor_jungle"}},
    "slime_block": {"kind": "block", "shape": "cube", "faces": {"all": "slime_block"}},
    "grass": {"kind": "block", "shape": "cube", "faces": {"top": "grass", "side": "grass_1", "bottom": "dirt"}},
    "grass_snowed": {"kind": "block", "shape": "cube", "faces": {"top": "snow", "side": "grass_2", "bottom": "dirt"}},
    "grass_path": {"kind": "block", "shape": "cube_path", "faces": {"top": "grass_path_top", "side": "grass_path_side", "bottom": "dirt"}},
    "mycelium": {"kind": "block", "shape": "cube", "faces": {"top": "mycelium_top", "side": "mycelium_side", "bottom": "dirt"}},
    "farmland": {"kind": "block", "shape": "cube_path", "faces": {"top": "farmland", "side": "dirt", "bottom": "dirt"}},
    "farmland_wet": {"kind": "block", "shape": "cube_path", "faces": {"top": "farmland_1", "side": "dirt", "bottom": "dirt"}},
    "furnace": {"kind": "block", "shape": "cube", "faces": {"top": "furnace_3", "bottom": "furnace_3", "front": "furnace", "side": "furnace_2"}},
    "furnace_lit": {"kind": "block", "shape": "cube", "faces": {"top": "furnace_3", "bottom": "furnace_3", "front": "furnace_1", "side": "furnace_2"}},
    "crafting_table": {"kind": "block", "shape": "cube", "faces": {"top": "crafting_table", "front": "crafting_table_1", "side": "crafting_table_2", "bottom": "planks"}},
    "tnt": {"kind": "block", "shape": "cube", "faces": {"side": "tnt", "top": "tnt_1", "bottom": "tnt_2"}},
    "bookshelf": {"kind": "block", "shape": "cube", "faces": {"side": "bookshelf", "top": "planks", "bottom": "planks"}},
    "jukebox": {"kind": "block", "shape": "cube", "faces": {"top": "jukebox_top", "side": "jukebox_side", "bottom": "planks"}},
    "pumpkin": {"kind": "block", "shape": "cube", "faces": {"top": "pumpkin", "bottom": "pumpkin", "side": "pumpkin_1", "front": "pumpkin_1"}},
    "pumpkin_carved": {"kind": "block", "shape": "cube", "faces": {"top": "pumpkin", "bottom": "pumpkin", "side": "pumpkin_1", "front": "pumpkin_2"}},
    "jack_o_lantern": {"kind": "block", "shape": "cube", "faces": {"top": "pumpkin", "bottom": "pumpkin", "side": "pumpkin_1", "front": "pumpkin_3"}},
    "melon": {"kind": "block", "shape": "cube", "faces": {"top": "melon_1", "bottom": "melon_1", "side": "melon"}},
    "cactus": {"kind": "block", "shape": "cube", "faces": {"top": "cactus", "side": "cactus_1", "bottom": "cactus_2"}},
    "hayblock": {"kind": "block", "shape": "cube", "faces": {"top": "hayblock_1", "bottom": "hayblock_1", "side": "hayblock"}},
    "stonecutter": {"kind": "block", "shape": "cube", "faces": {"top": "stonecutter_2", "bottom": "stonecutter_3", "front": "stonecutter", "side": "stonecutter_1"}},
    "stone_slab": {"kind": "block", "shape": "slab", "faces": {"top": "stone_slab", "bottom": "stone_slab", "side": "stone_slab_1"}},
    "double_stone_slab": {"kind": "block", "shape": "cube", "faces": {"top": "stone_slab", "bottom": "stone_slab", "side": "stone_slab_1"}},
    "daylight_detector": {"kind": "block", "shape": "slab_low", "faces": {"top": "daylight_detector_top", "side": "daylight_detector_side"}},
    "daylight_detector_inverted": {"kind": "block", "shape": "slab_low", "faces": {"top": "daylight_detector_inverted_top", "side": "daylight_detector_side"}},
    "ender_chest": {"kind": "block", "shape": "chest_box", "faces": {"top": "ender_chest_top", "front": "ender_chest_front", "side": "ender_chest_side"}},
    "chest": {"kind": "block", "shape": "chest_box", "faces": {"top": "chest_inventory", "front": "chest_inventory_2", "side": "chest_inventory_1"}},
    "enchanting_table": {"kind": "block", "shape": "slab", "faces": {"top": "enchanting_table_top", "side": "enchanting_table_side", "bottom": "enchanting_table_bottom"}},
    "endframe": {"kind": "block", "shape": "cube_path", "faces": {"top": "endframe_top", "side": "endframe_side", "bottom": "end_stone"}},
    "cauldron": {"kind": "block", "shape": "cube", "faces": {"top": "cauldron_top", "side": "cauldron_side", "bottom": "cauldron_bottom"}},
    "cake": {"kind": "block", "shape": "slab", "faces": {"top": "cake_top", "side": "cake_side", "bottom": "cake_bottom"}},
    "piston": {"kind": "block", "shape": "cube", "faces": {"top": "piston_top_normal", "side": "piston_side", "bottom": "piston_bottom"}},
    "sticky_piston": {"kind": "block", "shape": "cube", "faces": {"top": "piston_top_sticky", "side": "piston_side", "bottom": "piston_bottom"}},
    "reactor_core": {"kind": "block", "shape": "cube", "faces": {"all": "reactor_core_stage_x"}},
    "snow": {"kind": "block", "shape": "plate", "faces": {"all": "snow"}},
    "repeater_off": {"kind": "block", "shape": "plate", "faces": {"top": "repeater_off", "side": "stone_slab_1", "bottom": "stone"}},
    "repeater_on": {"kind": "block", "shape": "plate", "faces": {"top": "repeater_on", "side": "stone_slab_1", "bottom": "stone"}},
    "log_oak": {"kind": "block", "shape": "cube", "faces": {"side": "log", "top": "log_1", "bottom": "log_1"}},
    "log_birch": {"kind": "block", "shape": "cube", "faces": {"side": "log_2", "top": "log_3", "bottom": "log_3"}},
    "log_spruce": {"kind": "block", "shape": "cube", "faces": {"side": "log_4", "top": "log_5", "bottom": "log_5"}},
    "log_jungle": {"kind": "block", "shape": "cube", "faces": {"side": "log_6", "top": "log_7", "bottom": "log_7"}},
    "planks_oak": {"kind": "block", "shape": "cube", "faces": {"all": "planks"}},
    "planks_spruce": {"kind": "block", "shape": "cube", "faces": {"all": "planks_1"}},
    "planks_birch": {"kind": "block", "shape": "cube", "faces": {"all": "planks_2"}},
    "planks_jungle": {"kind": "block", "shape": "cube", "faces": {"all": "planks_3"}},
    "sandstone": {"kind": "block", "shape": "cube", "faces": {"side": "sandstone", "top": "sandstone_1", "bottom": "sandstone_2"}},
    "sandstone_chiseled": {"kind": "block", "shape": "cube", "faces": {"side": "sandstone_3", "top": "sandstone_1", "bottom": "sandstone_2"}},
    "sandstone_smooth": {"kind": "block", "shape": "cube", "faces": {"side": "sandstone_4", "top": "sandstone_1", "bottom": "sandstone_2"}},
    "quartz_block": {"kind": "block", "shape": "cube", "faces": {"top": "quartz_block", "side": "quartz_block_1", "bottom": "quartz_block_2"}},
    "quartz_block_chiseled": {"kind": "block", "shape": "cube", "faces": {"top": "quartz_block_3", "side": "quartz_block_4", "bottom": "quartz_block_3"}},
    "quartz_block_pillar": {"kind": "block", "shape": "cube", "faces": {"top": "quartz_block_5", "side": "quartz_block_6", "bottom": "quartz_block_5"}},
    "leaves": {"kind": "block", "shape": "cube", "faces": {"all": "leaves"}, "tint": GREEN_TINT},
    "leaves_1": {"kind": "block", "shape": "cube", "faces": {"all": "leaves_1"}, "tint": (97, 153, 97)},
    "leaves_2": {"kind": "block", "shape": "cube", "faces": {"all": "leaves_2"}, "tint": (128, 167, 85)},
    "leaves_3": {"kind": "block", "shape": "cube", "faces": {"all": "leaves_3"}, "tint": (89, 174, 48)},
    "leaves_opaque": {"kind": "block", "shape": "cube", "faces": {"all": "leaves_opaque"}, "tint": GREEN_TINT},
    "leaves_opaque_1": {"kind": "block", "shape": "cube", "faces": {"all": "leaves_opaque_1"}, "tint": (97, 153, 97)},
    "leaves_opaque_2": {"kind": "block", "shape": "cube", "faces": {"all": "leaves_opaque_2"}, "tint": (128, 167, 85)},
    "leaves_opaque_3": {"kind": "block", "shape": "cube", "faces": {"all": "leaves_opaque_3"}, "tint": (89, 174, 48)},
    "flower_rose": {"kind": "block", "shape": "cross", "faces": {"all": "flower_rose"}},
    "flower_rose_blue": {"kind": "block", "shape": "cross", "faces": {"all": "flower_rose_blue"}},
    "flower_dandelion": {"kind": "block", "shape": "cross", "faces": {"all": "flower_dandelion"}},
    "flower_paeonia": {"kind": "block", "shape": "cross", "faces": {"all": "flower_paeonia"}},
    "flower_oxeye_daisy": {"kind": "block", "shape": "cross", "faces": {"all": "flower_oxeye_daisy"}},
    "flower_houstonia": {"kind": "block", "shape": "cross", "faces": {"all": "flower_houstonia"}},
    "flower_blue_orchid": {"kind": "block", "shape": "cross", "faces": {"all": "flower_blue_orchid"}},
    "flower_allium": {"kind": "block", "shape": "cross", "faces": {"all": "flower_allium"}},
    "double_plant_rose": {"kind": "block", "shape": "double_cross", "faces": {"bottom": "double_plant_rose_bottom", "top": "double_plant_rose_top"}},
    "double_plant_paeonia": {"kind": "block", "shape": "double_cross", "faces": {"bottom": "double_plant_paeonia_bottom", "top": "double_plant_paeonia_top"}},
    "double_plant_grass": {"kind": "block", "shape": "double_cross", "faces": {"bottom": "double_plant_grass_bottom", "top": "double_plant_grass_top"}, "tint": GREEN_TINT},
    "double_plant_fern": {"kind": "block", "shape": "double_cross", "faces": {"bottom": "double_plant_fern_bottom", "top": "double_plant_fern_top"}, "tint": GREEN_TINT},
    "sapling": {"kind": "block", "shape": "cross", "faces": {"all": "sapling"}},
    "sapling_1": {"kind": "block", "shape": "cross", "faces": {"all": "sapling_1"}},
    "sapling_2": {"kind": "block", "shape": "cross", "faces": {"all": "sapling_2"}},
    "sapling_3": {"kind": "block", "shape": "cross", "faces": {"all": "sapling_3"}},
    "tallgrass": {"kind": "block", "shape": "cross", "faces": {"all": "tallgrass"}, "tint": GREEN_TINT},
    "tallgrass_1": {"kind": "block", "shape": "cross", "faces": {"all": "tallgrass_1"}},
    "tallgrass_2": {"kind": "block", "shape": "cross", "faces": {"all": "tallgrass_2"}, "tint": GREEN_TINT},
    "tallgrass_3": {"kind": "block", "shape": "cross", "faces": {"all": "tallgrass_3"}},
    "tallgrass_4": {"kind": "block", "shape": "cross", "faces": {"all": "tallgrass_4"}},
    "mushroom_red": {"kind": "block", "shape": "cross", "faces": {"all": "mushroom_red"}},
    "mushroom_brown": {"kind": "block", "shape": "cross", "faces": {"all": "mushroom_brown"}},
    "reeds": {"kind": "block", "shape": "cross", "faces": {"all": "reeds"}, "item_icon": "reeds"},
    "web": {"kind": "block", "shape": "cross", "faces": {"all": "web"}},
    "torch_on": {"kind": "block", "shape": "cross", "faces": {"all": "torch_on"}},
    "redstone_torch_on": {"kind": "block", "shape": "cross", "faces": {"all": "redstone_torch_on"}},
    "redstone_torch_off": {"kind": "block", "shape": "cross", "faces": {"all": "redstone_torch_off"}},
    "fire": {"kind": "block", "shape": "cross", "faces": {"all": "fire"}},
    "brewing_stand": {"kind": "block", "shape": "cross", "faces": {"all": "brewing_stand"}, "item_icon": "brewing_stand"},
    "waterlily": {"kind": "block", "shape": "carpet", "faces": {"all": "waterlily"}, "tint": (32, 128, 48)},
    "vine": {"kind": "block", "shape": "pane", "faces": {"all": "vine"}, "tint": GREEN_TINT},
    "ladder": {"kind": "block", "shape": "pane", "faces": {"all": "ladder"}},
    "sweet_berry_bush": {"kind": "block", "shape": "cross", "faces": {"all": "sweet_berry_bush_stage3"}},
    "sweet_berry_bush_stage0": {"kind": "block", "shape": "cross", "faces": {"all": "sweet_berry_bush_stage0"}},
    "sweet_berry_bush_stage1": {"kind": "block", "shape": "cross", "faces": {"all": "sweet_berry_bush_stage1"}},
    "sweet_berry_bush_stage2": {"kind": "block", "shape": "cross", "faces": {"all": "sweet_berry_bush_stage2"}},
    "sweet_berry_bush_stage3": {"kind": "block", "shape": "cross", "faces": {"all": "sweet_berry_bush_stage3"}},
    "flower_pot": {"kind": "item", "item_tex": "flower_pot", "block_tex": "flower_pot"},
    "sweet_berries": {"kind": "item", "item_tex": "sweet_berries"},
    "copper_ingot": {"kind": "item", "item_tex": "copper_ingot"},
    "oak_boat": {"kind": "item", "item_tex": "oak_boat"},
    "spruce_boat": {"kind": "item", "item_tex": "spruce_boat"},
    "birch_boat": {"kind": "item", "item_tex": "birch_boat"},
    "jungle_boat": {"kind": "item", "item_tex": "jungle_boat"},
    "armorstand": {"kind": "item", "item_tex": "armorstand"},
    "item_frame": {"kind": "item", "item_tex": "item_frame"},
}

for _hname in MOB_HEAD_SPECS:
    CATALOG[_hname] = {"kind": "head", "shape": "mob_head", "head_key": _hname}

BED_KEYS = [
    "bed",
    "bed_white", "bed_orange", "bed_magenta", "bed_light_blue",
    "bed_yellow", "bed_lime", "bed_pink", "bed_gray",
    "bed_silver", "bed_cyan", "bed_purple", "bed_blue",
    "bed_brown", "bed_green", "bed_red", "bed_black",
]
for _bkey in BED_KEYS:
    CATALOG[_bkey] = {"kind": "block", "shape": "bed", "bed_key": _bkey, "item_icon": _bkey}


class AssetLoader:
    def __init__(self) -> None:
        self.terrain_atlas: Optional[Image.Image] = None
        self.items_atlas: Optional[Image.Image] = None
        self.terrain_meta: Dict[str, List[float]] = {}
        self.items_meta: Dict[str, List[float]] = {}
        self._load_atlases()

    def _load_atlases(self) -> None:
        t_img = NA_ASSETS / "images" / "terrain-atlas.tga"
        t_meta = NA_ASSETS / "images" / "terrain.meta"
        if t_img.exists() and t_meta.exists():
            self.terrain_atlas = Image.open(t_img).convert("RGBA")
            self.terrain_meta = self._parse_meta(t_meta)
        i_img = NA_ASSETS / "images" / "items-opaque.png"
        i_meta = NA_ASSETS / "images" / "items.meta"
        if i_img.exists() and i_meta.exists():
            self.items_atlas = Image.open(i_img).convert("RGBA")
            self.items_meta = self._parse_meta(i_meta)

    @staticmethod
    def _parse_meta(meta_path: Path) -> Dict[str, List[float]]:
        out: Dict[str, List[float]] = {}
        try:
            data = json.loads(meta_path.read_text(encoding="utf-8"))
        except Exception:
            return out
        for entry in data:
            name = entry.get("name")
            uv = entry.get("uv")
            if name and isinstance(uv, list) and len(uv) >= 4 and uv[2] > 0.0001:
                out[name] = uv
            for idx, extra in enumerate(entry.get("additonal_textures", []), start=1):
                if isinstance(extra, list) and len(extra) >= 4 and extra[2] > 0.0001:
                    out[f"{name}_{idx}"] = extra
        return out

    @staticmethod
    def _first_frame(img: Image.Image) -> Image.Image:
        rgba = img.convert("RGBA")
        if rgba.height > rgba.width and rgba.height % rgba.width == 0:
            return rgba.crop((0, 0, rgba.width, rgba.width))
        return rgba

    def load_block(self, name: str) -> Optional[Image.Image]:
        for d in SEARCH_BLOCK_DIRS:
            p = d / f"{name}.png"
            if p.exists():
                return self._first_frame(Image.open(p))
        if self.terrain_atlas is not None and name in self.terrain_meta:
            return self._first_frame(self._slice_uv(self.terrain_atlas, self.terrain_meta[name]))
        return None

    def load_item(self, name: str) -> Optional[Image.Image]:
        for d in SEARCH_ITEM_DIRS:
            p = d / f"{name}.png"
            if p.exists():
                return self._first_frame(Image.open(p))
        if self.items_atlas is not None and name in self.items_meta:
            return self._first_frame(self._slice_uv(self.items_atlas, self.items_meta[name]))
        return None

    def load_mob(self, filename: str) -> Optional[Image.Image]:
        for d in SEARCH_MOB_DIRS:
            p = d / filename
            if p.exists():
                return Image.open(p).convert("RGBA")
        return None

    @staticmethod
    def _slice_uv(atlas: Image.Image, uv: List[float]) -> Image.Image:
        aw, ah = atlas.size
        if len(uv) >= 6 and uv[4] > 0 and uv[5] > 0:
            aw_ref, ah_ref = float(uv[4]), float(uv[5])
        else:
            aw_ref, ah_ref = float(aw), float(ah)
        x0 = max(0, min(aw - 1, int(round(uv[0] * aw_ref * (aw / aw_ref)))))
        y0 = max(0, min(ah - 1, int(round(uv[1] * ah_ref * (ah / ah_ref)))))
        x1 = max(x0 + 1, min(aw, int(round(uv[2] * aw_ref * (aw / aw_ref)))))
        y1 = max(y0 + 1, min(ah, int(round(uv[3] * ah_ref * (ah / ah_ref)))))
        return atlas.crop((x0, y0, x1, y1)).convert("RGBA")


def _solve_3x3(A: List[List[float]], b: List[float]) -> List[float]:
    m = [row[:] + [val] for row, val in zip(A, b)]
    for col in range(3):
        piv = max(range(col, 3), key=lambda r: abs(m[r][col]))
        m[col], m[piv] = m[piv], m[col]
        if abs(m[col][col]) < 1e-12:
            return [0.0, 0.0, 0.0]
        div = m[col][col]
        for j in range(col, 4):
            m[col][j] /= div
        for r in range(3):
            if r != col:
                factor = m[r][col]
                for j in range(col, 4):
                    m[r][j] -= factor * m[col][j]
    return [m[0][3], m[1][3], m[2][3]]


def _shade_image(img: Image.Image, shade: float) -> Image.Image:
    if abs(shade - 1.0) < 1e-3:
        return img
    r, g, b, a = img.split()
    lut = [min(255, max(0, int(round(i * shade)))) for i in range(256)]
    return Image.merge("RGBA", (r.point(lut), g.point(lut), b.point(lut), a))


def _apply_tint_if_gray(img: Image.Image, tint: Optional[Tuple[int, int, int]]) -> Image.Image:
    if tint is None:
        return img
    rgba = img.convert("RGBA")
    px = rgba.load()
    is_gray = True
    for y in range(rgba.height):
        for x in range(rgba.width):
            r, g, b, a = px[x, y]
            if a > 16 and (abs(r - g) > 12 or abs(g - b) > 12 or abs(r - b) > 12):
                is_gray = False
                break
        if not is_gray:
            break
    if not is_gray:
        return rgba
    tr, tg, tb = tint
    out = Image.new("RGBA", rgba.size)
    op = out.load()
    for y in range(rgba.height):
        for x in range(rgba.width):
            r, g, b, a = px[x, y]
            op[x, y] = ((r * tr) // 255, (g * tg) // 255, (b * tb) // 255, a)
    return out


def _project_iso(
    pt: Tuple[float, float, float],
    yaw_deg: float,
    pitch_deg: float,
    scale: float,
    cx: float,
    cy: float,
    flip_y: bool = False,
) -> Tuple[float, float, float]:
    x, y, z = pt
    if flip_y:
        y = -y
    yr = math.radians(yaw_deg)
    pr = math.radians(pitch_deg)
    cy_v, sy_v = math.cos(yr), math.sin(yr)
    cp_v, sp_v = math.cos(pr), math.sin(pr)
    xr = x * cy_v - z * sy_v
    zr = x * sy_v + z * cy_v
    yr2 = y * cp_v - zr * sp_v
    zr2 = y * sp_v + zr * cp_v
    sx = cx + xr * scale
    sy = cy - yr2 * scale
    return (sx, sy, zr2)


def _draw_textured_quad(
    canvas: Image.Image,
    tex: Image.Image,
    pts_2d: List[Tuple[float, float]],
    uv_pixels: Optional[Tuple[float, float, float, float]] = None,
    shade: float = 1.0,
) -> None:
    if len(pts_2d) != 4:
        return
    if uv_pixels is not None:
        u0, v0, u1, v1 = uv_pixels
        x0, x1 = min(u0, u1), max(u0, u1)
        y0, y1 = min(v0, v1), max(v0, v1)
        ix0 = max(0, min(tex.width - 1, int(math.floor(x0))))
        iy0 = max(0, min(tex.height - 1, int(math.floor(y0))))
        ix1 = max(ix0 + 1, min(tex.width, int(math.ceil(x1))))
        iy1 = max(iy0 + 1, min(tex.height, int(math.ceil(y1))))
        cropped = tex.crop((ix0, iy0, ix1, iy1))
        if u0 > u1:
            cropped = cropped.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
        if v0 > v1:
            cropped = cropped.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
        src = cropped
    else:
        src = tex

    src = _shade_image(src, shade)
    tw, th = src.size
    padded = Image.new("RGBA", (tw + 2, th + 2))
    padded.paste(src, (1, 1))
    padded.paste(src.crop((0, 0, tw, 1)), (1, 0))
    padded.paste(src.crop((0, th - 1, tw, th)), (1, th + 1))
    padded.paste(src.crop((0, 0, 1, th)), (0, 1))
    padded.paste(src.crop((tw - 1, 0, tw, th)), (tw + 1, 1))
    padded.putpixel((0, 0), src.getpixel((0, 0)))
    padded.putpixel((tw + 1, 0), src.getpixel((tw - 1, 0)))
    padded.putpixel((0, th + 1), src.getpixel((0, th - 1)))
    padded.putpixel((tw + 1, th + 1), src.getpixel((tw - 1, th - 1)))

    su = [(1.0, 1.0), (1.0 + tw, 1.0), (1.0 + tw, 1.0 + th)]
    dst3 = pts_2d[:3]
    A = [[p[0], p[1], 1.0] for p in dst3]
    abc = _solve_3x3(A, [u[0] for u in su])
    def_ = _solve_3x3(A, [u[1] for u in su])

    warped = padded.transform(
        canvas.size,
        Image.Transform.AFFINE,
        (abc[0], abc[1], abc[2], def_[0], def_[1], def_[2]),
        resample=Image.Resampling.NEAREST,
    )
    mask = Image.new("L", canvas.size, 0)
    ImageDraw.Draw(mask).polygon(pts_2d, fill=255, outline=255)
    w_r, w_g, w_b, w_a = warped.split()
    from PIL import ImageChops

    masked_a = ImageChops.multiply(w_a, mask)
    warped = Image.merge("RGBA", (w_r, w_g, w_b, masked_a))
    canvas.alpha_composite(warped)


def _normal_shade(pts3d: List[Tuple[float, float, float]]) -> float:
    ax, ay, az = (
        pts3d[1][0] - pts3d[0][0],
        pts3d[1][1] - pts3d[0][1],
        pts3d[1][2] - pts3d[0][2],
    )
    bx, by, bz = (
        pts3d[3][0] - pts3d[0][0],
        pts3d[3][1] - pts3d[0][1],
        pts3d[3][2] - pts3d[0][2],
    )
    nx = ay * bz - az * by
    ny = az * bx - ax * bz
    nz = ax * by - ay * bx
    length = math.sqrt(nx * nx + ny * ny + nz * nz) or 1.0
    nx, ny, nz = nx / length, ny / length, nz / length
    if abs(ny) > 0.7:
        return 1.0 if ny > 0 else 0.5
    if abs(nx) > abs(nz):
        return 0.65 if nx > 0 else 0.80
    return 0.80 if nz > 0 else 0.65


def _box_faces(
    x0: float,
    y0: float,
    z0: float,
    x1: float,
    y1: float,
    z1: float,
    tex_map: Dict[str, Tuple[Image.Image, Optional[Tuple[float, float, float, float]], float]],
) -> List[dict]:
    faces_def = [
        ("top", [(x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1)], (0, 1, 0), 1.0),
        ("bottom", [(x0, y0, z1), (x1, y0, z1), (x1, y0, z0), (x0, y0, z0)], (0, -1, 0), 0.50),
        ("front", [(x0, y1, z1), (x1, y1, z1), (x1, y0, z1), (x0, y0, z1)], (0, 0, 1), 0.82),
        ("back", [(x1, y1, z0), (x0, y1, z0), (x0, y0, z0), (x1, y0, z0)], (0, 0, -1), 0.62),
        ("right", [(x1, y1, z1), (x1, y1, z0), (x1, y0, z0), (x1, y0, z1)], (1, 0, 0), 0.65),
        ("left", [(x0, y1, z0), (x0, y1, z1), (x0, y0, z1), (x0, y0, z0)], (-1, 0, 0), 0.80),
    ]
    out = []
    for fname, pts, norm, default_shade in faces_def:
        entry = tex_map.get(fname) or tex_map.get("side") or tex_map.get("all")
        if entry is None:
            continue
        tex, uv, sh = entry
        out.append({"pts": pts, "norm": norm, "tex": tex, "uv": uv, "shade": sh if sh > 0 else default_shade})
    return out


def _render_faces_iso(
    faces: List[dict],
    size: int,
    yaw_deg: float = 45.0,
    pitch_deg: float = 30.0,
    scale_mul: float = 0.58,
    flip_y: bool = False,
    cull_backfaces: bool = True,
) -> Image.Image:
    yr = math.radians(yaw_deg)
    pr = math.radians(pitch_deg)
    cy_v, sy_v = math.cos(yr), math.sin(yr)
    cp_v, sp_v = math.cos(pr), math.sin(pr)

    all_pts = [p for f in faces for p in f["pts"]]
    cx3 = 0.5 * (min(p[0] for p in all_pts) + max(p[0] for p in all_pts))
    cy3 = 0.5 * (min(p[1] for p in all_pts) + max(p[1] for p in all_pts))
    cz3 = 0.5 * (min(p[2] for p in all_pts) + max(p[2] for p in all_pts))

    raw_proj = []
    for f in faces:
        p3_centered = [(p[0] - cx3, (-(p[1] - cy3) if flip_y else (p[1] - cy3)), p[2] - cz3) for p in f["pts"]]
        nx, ny, nz = f["norm"]
        if flip_y:
            ny = -ny
        nrx = nx * cy_v - nz * sy_v
        nrz = nx * sy_v + nz * cy_v
        nrz2 = ny * sp_v + nrz * cp_v
        if cull_backfaces and nrz2 <= 1e-4:
            continue
        projected = [_project_iso(p, yaw_deg, pitch_deg, 1.0, 0.0, 0.0, flip_y=False) for p in p3_centered]
        avg_z = sum(p[2] for p in projected) / len(projected)
        raw_proj.append((avg_z, f, [(p[0], p[1]) for p in projected]))

    if not raw_proj:
        return Image.new("RGBA", (size, size), (0, 0, 0, 0))

    xs = [q[0] for _, _, pts in raw_proj for q in pts]
    ys = [q[1] for _, _, pts in raw_proj for q in pts]
    span = max(max(xs) - min(xs), max(ys) - min(ys), 1e-4)
    scale = (size * scale_mul) / span
    mid_x = 0.5 * (min(xs) + max(xs))
    mid_y = 0.5 * (min(ys) + max(ys))

    raw_proj.sort(key=lambda item: (item[1].get("layer", 0), item[0]))
    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    for _, f, pts2 in raw_proj:
        screen_pts = [((p[0] - mid_x) * scale + size * 0.5, (p[1] - mid_y) * scale + size * 0.5) for p in pts2]
        _draw_textured_quad(canvas, f["tex"], screen_pts, f.get("uv"), f["shade"])
    return canvas


def render_3d_block(loader: AssetLoader, spec: dict, size: int) -> Optional[Image.Image]:
    shape = spec["shape"]
    if shape == "mob_head":
        return render_3d_mob_head(loader, spec["head_key"], size)

    if shape == "bed":
        bkey = spec.get("bed_key", "bed")
        t_foot_top = loader.load_block(bkey)
        t_foot_end = loader.load_block(f"{bkey}_1")
        t_foot_side = loader.load_block(f"{bkey}_2")
        t_head_top = loader.load_block(f"{bkey}_3")
        t_head_end = loader.load_block(f"{bkey}_4")
        t_head_side = loader.load_block(f"{bkey}_5")
        if not all((t_foot_top, t_foot_end, t_foot_side, t_head_top, t_head_end, t_head_side)):
            return None

        def _clean_alpha(im: Image.Image) -> Image.Image:
            out = im.convert("RGBA").copy()
            px = out.load()
            for yy in range(out.height):
                for xx in range(out.width):
                    r, g, b, a = px[xx, yy]
                    px[xx, yy] = (r, g, b, 255 if a >= 128 else 0)
            return out

        t_foot_end = _clean_alpha(t_foot_end)
        t_foot_side = _clean_alpha(t_foot_side)
        t_head_end = _clean_alpha(t_head_end)
        t_head_side = _clean_alpha(t_head_side)
        t_head_top_rot = t_head_top.transpose(Image.Transpose.ROTATE_90)
        t_foot_top_rot = t_foot_top.transpose(Image.Transpose.ROTATE_90)
        w, h = float(t_foot_side.width), float(t_foot_side.height)
        v_top = h * (7.0 / 16.0)
        y0 = -0.5
        y1 = -0.5 + (9.0 / 16.0)
        faces = []
        faces.extend(_box_faces(-0.5, y0, -1.0, 0.5, y1, 0.0, {
            "top": (t_head_top_rot, None, 1.0),
            "back": (t_head_end, (0.0, v_top, w, h), 0.70),
            "left": (t_head_side, (w, v_top, 0.0, h), 0.75),
            "right": (t_head_side, (0.0, v_top, w, h), 0.65),
        }))
        faces.extend(_box_faces(-0.5, y0, 0.0, 0.5, y1, 1.0, {
            "top": (t_foot_top_rot, None, 1.0),
            "front": (t_foot_end, (0.0, v_top, w, h), 0.80),
            "left": (t_foot_side, (w, v_top, 0.0, h), 0.75),
            "right": (t_foot_side, (0.0, v_top, w, h), 0.65),
        }))
        return _render_faces_iso(faces, size, yaw_deg=45.0, pitch_deg=35.0, scale_mul=0.82)

    faces_spec = spec.get("faces", {})
    tint = spec.get("tint")

    def get_t(key: str, fallback: str = "all") -> Optional[Image.Image]:
        tname = faces_spec.get(key) or faces_spec.get(fallback) or faces_spec.get("all")
        tex = loader.load_block(tname) if tname else None
        if tex is not None and tint is not None and (key in ("top", "all", "bottom") or faces_spec.get(key) == faces_spec.get("all")):
            tex = _apply_tint_if_gray(tex, tint)
        return tex

    if shape == "cross":
        tex = get_t("all")
        if tex is None:
            return None
        d = 0.3535
        p000 = (-d, -0.5, -d)
        p101 = (d, -0.5, d)
        p111 = (d, 0.5, d)
        p010 = (-d, 0.5, -d)
        p100 = (d, -0.5, -d)
        p001 = (-d, -0.5, d)
        p011 = (-d, 0.5, d)
        p110 = (d, 0.5, -d)
        mid0 = (0.0, -0.5, 0.0)
        mid1 = (0.0, 0.5, 0.0)
        hw = tex.width * 0.5
        th = float(tex.height)
        faces = [
            {"pts": [p010, mid1, mid0, p000], "norm": (0, 0, 1), "tex": tex, "uv": (0, 0, hw, th), "shade": 0.90},
            {"pts": [mid1, p111, p101, mid0], "norm": (0, 0, 1), "tex": tex, "uv": (hw, 0, tex.width, th), "shade": 0.90},
            {"pts": [p011, mid1, mid0, p001], "norm": (0, 0, 1), "tex": tex, "uv": (0, 0, hw, th), "shade": 1.0},
            {"pts": [mid1, p110, p100, mid0], "norm": (0, 0, 1), "tex": tex, "uv": (hw, 0, tex.width, th), "shade": 1.0},
        ]
        return _render_faces_iso(faces, size, yaw_deg=16.0, pitch_deg=16.0, scale_mul=0.80, cull_backfaces=False)

    if shape == "double_cross":
        tex_b = get_t("bottom")
        tex_t = get_t("top")
        if tex_b is None or tex_t is None:
            return None
        d = 0.3535
        faces = []
        for idx, tex in enumerate((tex_b, tex_t)):
            y0, y1 = (-1.0 + idx), (0.0 + idx)
            hw, th = tex.width * 0.5, float(tex.height)
            faces.extend([
                {"pts": [(-d, y1, -d), (0, y1, 0), (0, y0, 0), (-d, y0, -d)], "norm": (0, 0, 1), "tex": tex, "uv": (0, 0, hw, th), "shade": 0.90},
                {"pts": [(0, y1, 0), (d, y1, d), (d, y0, d), (0, y0, 0)], "norm": (0, 0, 1), "tex": tex, "uv": (hw, 0, tex.width, th), "shade": 0.90},
                {"pts": [(-d, y1, d), (0, y1, 0), (0, y0, 0), (-d, y0, d)], "norm": (0, 0, 1), "tex": tex, "uv": (0, 0, hw, th), "shade": 1.0},
                {"pts": [(0, y1, 0), (d, y1, -d), (d, y0, -d), (0, y0, 0)], "norm": (0, 0, 1), "tex": tex, "uv": (hw, 0, tex.width, th), "shade": 1.0},
            ])
        return _render_faces_iso(faces, size, yaw_deg=16.0, pitch_deg=16.0, scale_mul=0.86, cull_backfaces=False)

    if shape == "carpet":
        tex = get_t("all")
        if tex is None:
            return None
        faces = _box_faces(-0.5, -0.5, -0.5, 0.5, -0.4375, 0.5, {
            "top": (tex, None, 1.0),
            "front": (tex, (0, float(tex.height) - 1.0, float(tex.width), float(tex.height)), 0.80),
            "left": (tex, (0, float(tex.height) - 1.0, float(tex.width), float(tex.height)), 0.72),
            "right": (tex, (0, float(tex.height) - 1.0, float(tex.width), float(tex.height)), 0.65),
        })
        return _render_faces_iso(faces, size, yaw_deg=45.0, pitch_deg=35.0, scale_mul=0.72)

    if shape == "pane":
        tex = get_t("all")
        if tex is None:
            return None
        w, h = float(tex.width), float(tex.height)
        faces = _box_faces(-0.5, -0.5, -0.04, 0.5, 0.5, 0.04, {
            "front": (tex, None, 1.0),
            "back": (tex, None, 0.85),
            "top": (tex, (0, 0, w, 1.0), 0.92),
            "left": (tex, (0, 0, 1.0, h), 0.78),
            "right": (tex, (w - 1.0, 0, w, h), 0.70),
        })
        return _render_faces_iso(faces, size, yaw_deg=26.0, pitch_deg=18.0, scale_mul=0.76, cull_backfaces=True)

    if shape == "door":
        t_bot = get_t("bottom")
        t_top = get_t("top")
        if t_bot is None or t_top is None:
            return None
        wb, hb = float(t_bot.width), float(t_bot.height)
        wt, ht = float(t_top.width), float(t_top.height)
        faces = _box_faces(-0.5, -1.0, -0.09, 0.5, 0.0, 0.09, {
            "front": (t_bot, None, 1.0),
            "back": (t_bot, None, 0.85),
            "left": (t_bot, (0, 0, 2.0, hb), 0.78),
            "right": (t_bot, (wb - 2.0, 0, wb, hb), 0.70),
        }) + _box_faces(-0.5, 0.0, -0.09, 0.5, 1.0, 0.09, {
            "front": (t_top, None, 1.0),
            "back": (t_top, None, 0.85),
            "top": (t_top, (0, 0, wt, 2.0), 0.95),
            "left": (t_top, (0, 0, 2.0, ht), 0.78),
            "right": (t_top, (wt - 2.0, 0, wt, ht), 0.70),
        })
        return _render_faces_iso(faces, size, yaw_deg=26.0, pitch_deg=18.0, scale_mul=0.84, cull_backfaces=True)

    if shape in ("cube", "cube_path", "slab", "slab_low", "plate", "chest_box"):
        t_top = get_t("top")
        t_side = get_t("side")
        t_bot = get_t("bottom", "side")
        t_front = get_t("front", "side")
        if t_top is None or t_side is None:
            return None
        y0, y1 = -0.5, 0.5
        if shape == "slab":
            y0, y1 = -0.5, 0.0
        elif shape == "slab_low":
            y0, y1 = -0.5, -0.125
        elif shape == "plate":
            y0, y1 = -0.5, -0.3125
        elif shape == "cube_path":
            y0, y1 = -0.5, 0.4375
        elif shape == "chest_box":
            x0, x1, y0, y1, z0, z1 = -0.4375, 0.4375, -0.5, 0.375, -0.4375, 0.4375
            uv_top = (1, 1, 15, 15) if t_top.width == 16 and t_top.height == 16 else None
            uv_f = (1, 1, 15, 15) if t_front.width == 16 and t_front.height == 16 else None
            uv_s = (1, 1, 15, 15) if t_side.width == 16 and t_side.height == 16 else None
            tmap = {
                "top": (t_top, uv_top, 1.0),
                "front": (t_front, uv_f, 0.80),
                "left": (t_side, uv_s, 0.80),
                "right": (t_side, uv_s, 0.65),
            }
            faces = _box_faces(x0, y0, z0, x1, y1, z1, tmap)
            return _render_faces_iso(faces, size, yaw_deg=45.0, pitch_deg=30.0, scale_mul=0.68)

        uv_side = (0, (1.0 - (y1 - y0)) * t_side.height, t_side.width, t_side.height) if (y1 - y0) < 0.99 else None
        tmap = {
            "top": (t_top, None, 1.0),
            "bottom": (t_bot, None, 0.5),
            "front": (t_front, uv_side, 0.82),
            "left": (t_side, uv_side, 0.80),
            "right": (t_side, uv_side, 0.65),
            "back": (t_side, uv_side, 0.62),
        }
        faces = _box_faces(-0.5, y0, -0.5, 0.5, y1, 0.5, tmap)
        return _render_faces_iso(faces, size, yaw_deg=45.0, pitch_deg=30.0, scale_mul=0.68)

    if shape == "stairs":
        tex = get_t("all")
        if tex is None:
            return None
        w, h = float(tex.width), float(tex.height)
        bmap = {
            "top": (tex, (0, 0, w, h * 0.5), 1.0),
            "front": (tex, (0, h * 0.5, w, h), 0.82),
            "left": (tex, (0, h * 0.5, w, h), 0.80),
            "right": (tex, (0, h * 0.5, w, h), 0.65),
        }
        umap = {
            "top": (tex, (0, 0, w, h * 0.5), 1.0),
            "front": (tex, (0, 0, w, h * 0.5), 0.82),
            "left": (tex, (0, 0, w * 0.5, h * 0.5), 0.80),
            "right": (tex, (w * 0.5, 0, w, h * 0.5), 0.65),
        }
        faces = _box_faces(-0.5, -0.5, -0.5, 0.5, 0.0, 0.5, bmap) + _box_faces(-0.5, 0.0, -0.5, 0.5, 0.5, 0.0, umap)
        return _render_faces_iso(faces, size, yaw_deg=45.0, pitch_deg=30.0, scale_mul=0.68)

    return None


def _cube_box_faces(
    tex: Image.Image,
    origin: Tuple[float, float, float],
    dims: Tuple[float, float, float],
    uv_off: Tuple[int, int],
    uv_dims: Optional[Tuple[float, float, float]] = None,
    layer: int = 0,
) -> List[dict]:
    ox, oy, oz = origin
    w, h, d = dims
    uw, uh, ud = uv_dims if uv_dims is not None else dims
    u, v = uv_off
    x0, x1 = ox, ox + w
    y0, y1 = oy, oy + h
    z0, z1 = oz, oz + d
    return [
        {"pts": [(x1, y0, z1), (x0, y0, z1), (x0, y0, z0), (x1, y0, z0)], "norm": (0, -1, 0), "tex": tex, "uv": (u + ud, v, u + ud + uw, v + ud), "shade": 1.0, "layer": layer},
        {"pts": [(x1, y1, z0), (x0, y1, z0), (x0, y1, z1), (x1, y1, z1)], "norm": (0, 1, 0), "tex": tex, "uv": (u + ud + uw, v + ud, u + ud + 2 * uw, v), "shade": 0.55, "layer": layer},
        {"pts": [(x1, y0, z0), (x0, y0, z0), (x0, y1, z0), (x1, y1, z0)], "norm": (0, 0, -1), "tex": tex, "uv": (u + ud, v + ud, u + ud + uw, v + ud + uh), "shade": 0.92, "layer": layer},
        {"pts": [(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)], "norm": (0, 0, 1), "tex": tex, "uv": (u + 2 * ud + uw, v + ud, u + 2 * ud + 2 * uw, v + ud + uh), "shade": 0.65, "layer": layer},
        {"pts": [(x1, y0, z1), (x1, y0, z0), (x1, y1, z0), (x1, y1, z1)], "norm": (1, 0, 0), "tex": tex, "uv": (u, v + ud, u + ud, v + ud + uh), "shade": 0.72, "layer": layer},
        {"pts": [(x0, y0, z0), (x0, y0, z1), (x0, y1, z1), (x0, y1, z0)], "norm": (-1, 0, 0), "tex": tex, "uv": (u + ud + uw, v + ud, u + 2 * ud + uw, v + ud + uh), "shade": 0.80, "layer": layer},
    ]


def render_3d_mob_head(loader: AssetLoader, head_key: str, size: int) -> Optional[Image.Image]:
    spec = MOB_HEAD_SPECS.get(head_key)
    if not spec:
        return None
    skin = loader.load_mob(spec["skin"])
    if skin is None:
        return None
    faces: List[dict] = []
    for box in spec["boxes"]:
        origin, dims, uv_off = box[0], box[1], box[2]
        uv_dims = box[3] if len(box) > 3 else None
        layer = box[4] if len(box) > 4 else 0
        faces.extend(_cube_box_faces(skin, origin, dims, uv_off, uv_dims=uv_dims, layer=layer))
    return _render_faces_iso(faces, size, yaw_deg=145.0, pitch_deg=25.0, scale_mul=0.68, flip_y=True, cull_backfaces=True)


def render_2d_mob_head(loader: AssetLoader, head_key: str, size: int) -> Optional[Image.Image]:
    spec = MOB_HEAD_SPECS.get(head_key)
    if not spec:
        return None
    skin = loader.load_mob(spec["skin"])
    if skin is None:
        item_tex = loader.load_item(f"skull_{head_key.replace('head_', '')}")
        return item_tex.resize((size, size), Image.Resampling.NEAREST) if item_tex is not None else None
    u0, v0, u1, v1 = spec["face_uv"]
    face = skin.crop((u0, v0, u1, v1)).convert("RGBA")
    if "hat_uv" in spec:
        hu0, hv0, hu1, hv1 = spec["hat_uv"]
        hat = skin.crop((hu0, hv0, hu1, hv1)).convert("RGBA")
        face.alpha_composite(hat)
    if head_key == "head_fox":
        ear_l = skin.crop((9, 2, 11, 4)).convert("RGBA")
        ear_r = skin.crop((16, 2, 18, 4)).convert("RGBA")
        snout = skin.crop((9, 21, 13, 23)).convert("RGBA")
        comp = Image.new("RGBA", (8, 8), (0, 0, 0, 0))
        comp.paste(ear_l, (0, 0), ear_l)
        comp.paste(ear_r, (6, 0), ear_r)
        comp.paste(face, (0, 2), face)
        comp.paste(snout, (2, 6), snout)
        face = comp
    elif head_key == "head_pig":
        snout = skin.crop((17, 17, 21, 20)).convert("RGBA")
        face.paste(snout, (2, 4), snout)
    elif head_key == "head_villager":
        nose = skin.crop((26, 2, 28, 6)).convert("RGBA")
        comp = Image.new("RGBA", (8, 11), (0, 0, 0, 0))
        comp.paste(face, (0, 0), face)
        comp.paste(nose, (3, 7), nose)
        face = comp
    elif head_key == "head_wolf":
        ear_l = skin.crop((17, 15, 19, 17)).convert("RGBA")
        ear_r = skin.crop((17, 15, 19, 17)).convert("RGBA")
        snout = skin.crop((4, 14, 7, 17)).convert("RGBA")
        comp = Image.new("RGBA", (6, 8), (0, 0, 0, 0))
        comp.paste(ear_l, (0, 0), ear_l)
        comp.paste(ear_r, (4, 0), ear_r)
        comp.paste(face, (0, 2), face)
        comp.paste(snout, (1, 5), snout)
        face = comp
    elif head_key == "head_polarbear":
        ear = skin.crop((27, 1, 29, 3)).convert("RGBA")
        snout = skin.crop((3, 47, 8, 50)).convert("RGBA")
        comp = Image.new("RGBA", (9, 8), (0, 0, 0, 0))
        comp.paste(ear, (0, 0), ear)
        comp.paste(ear, (7, 0), ear)
        comp.paste(face, (1, 1), face)
        comp.paste(snout, (2, 5), snout)
        face = comp
    elif head_key == "head_ocelot":
        ear = skin.crop((3, 18, 4, 19)).convert("RGBA")
        snout = skin.crop((2, 25, 5, 27)).convert("RGBA")
        comp = Image.new("RGBA", (5, 5), (0, 0, 0, 0))
        comp.paste(ear, (0, 0), ear)
        comp.paste(ear, (4, 0), ear)
        comp.paste(face, (0, 1), face)
        comp.paste(snout, (1, 3), snout)
        face = comp
    elif head_key == "head_cow":
        horn = skin.crop((23, 1, 24, 4)).convert("RGBA")
        comp = Image.new("RGBA", (10, 9), (0, 0, 0, 0))
        comp.paste(horn, (0, 0), horn)
        comp.paste(horn, (9, 0), horn)
        comp.paste(face, (1, 1), face)
        face = comp
    elif head_key == "head_chicken":
        beak = skin.crop((16, 2, 20, 4)).convert("RGBA")
        wattle = skin.crop((16, 6, 18, 8)).convert("RGBA")
        comp = Image.new("RGBA", (4, 7), (0, 0, 0, 0))
        comp.paste(face, (0, 0), face)
        comp.paste(beak, (0, 2), beak)
        comp.paste(wattle, (1, 4), wattle)
        face = comp

    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    fw, fh = face.size
    pad = int(round(size * 0.88))
    ratio = min(pad / max(1, fw), pad / max(1, fh))
    rw, rh = max(1, int(round(fw * ratio))), max(1, int(round(fh * ratio)))
    resized = face.resize((rw, rh), Image.Resampling.NEAREST)
    canvas.paste(resized, ((size - rw) // 2, (size - rh) // 2), resized)
    return canvas


def render_3d_extruded_item(tex: Image.Image, size: int) -> Image.Image:
    rgba = tex.convert("RGBA")
    if max(rgba.size) > 32:
        rgba = rgba.copy()
        rgba.thumbnail((32, 32), Image.Resampling.NEAREST)
    w, h = rgba.size
    px = rgba.load()
    faces: List[dict] = []
    d = 0.5 / max(w, h)
    step_x = 1.0 / w
    step_y = 1.0 / h

    def solid(x: int, y: int) -> bool:
        return 0 <= x < w and 0 <= y < h and px[x, y][3] >= 32

    faces.append({
        "pts": [(-0.5, 0.5, d), (0.5, 0.5, d), (0.5, -0.5, d), (-0.5, -0.5, d)],
        "norm": (0, 0, 1),
        "tex": rgba,
        "uv": (0, 0, float(w), float(h)),
        "shade": 1.0,
    })
    faces.append({
        "pts": [(0.5, 0.5, -d), (-0.5, 0.5, -d), (-0.5, -0.5, -d), (0.5, -0.5, -d)],
        "norm": (0, 0, -1),
        "tex": rgba,
        "uv": (float(w), 0, 0, float(h)),
        "shade": 0.72,
    })

    for py in range(h):
        for px_x in range(w):
            if not solid(px_x, py):
                continue
            x0 = -0.5 + px_x * step_x
            x1 = x0 + step_x
            y1 = 0.5 - py * step_y
            y0 = y1 - step_y
            uv = (float(px_x), float(py), float(px_x + 1), float(py + 1))
            if not solid(px_x + 1, py):
                faces.append({"pts": [(x1, y1, d), (x1, y1, -d), (x1, y0, -d), (x1, y0, d)], "norm": (1, 0, 0), "tex": rgba, "uv": uv, "shade": 0.65})
            if not solid(px_x - 1, py):
                faces.append({"pts": [(x0, y1, -d), (x0, y1, d), (x0, y0, d), (x0, y0, -d)], "norm": (-1, 0, 0), "tex": rgba, "uv": uv, "shade": 0.80})
            if not solid(px_x, py - 1):
                faces.append({"pts": [(x0, y1, -d), (x1, y1, -d), (x1, y1, d), (x0, y1, d)], "norm": (0, 1, 0), "tex": rgba, "uv": uv, "shade": 0.95})
            if not solid(px_x, py + 1):
                faces.append({"pts": [(x0, y0, d), (x1, y0, d), (x1, y0, -d), (x0, y0, -d)], "norm": (0, -1, 0), "tex": rgba, "uv": uv, "shade": 0.55})

    return _render_faces_iso(faces, size, yaw_deg=28.0, pitch_deg=22.0, scale_mul=0.72, cull_backfaces=True)


def _square_pad_resize(img: Image.Image, size: int) -> Image.Image:
    w, h = img.size
    if w == h:
        return img.resize((size, size), Image.Resampling.NEAREST)
    ratio = size / max(w, h)
    rw, rh = max(1, int(round(w * ratio))), max(1, int(round(h * ratio)))
    resized = img.resize((rw, rh), Image.Resampling.NEAREST)
    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    canvas.paste(resized, ((size - rw) // 2, (size - rh) // 2), resized)
    return canvas


def render_2d_asset(loader: AssetLoader, name: str, spec: dict, size: int) -> Optional[Image.Image]:
    if spec["kind"] == "head":
        return render_2d_mob_head(loader, spec["head_key"], size)
    if spec["kind"] == "item":
        t = loader.load_item(spec["item_tex"]) or (loader.load_block(spec.get("block_tex", "")) if "block_tex" in spec else None)
        return _square_pad_resize(t, size) if t is not None else None
    faces = spec.get("faces", {})
    tint = spec.get("tint")
    if spec.get("item_icon"):
        ti = loader.load_item(spec["item_icon"])
        if ti is not None:
            return _square_pad_resize(ti, size)
    if spec.get("shape") in ("double_cross", "door") and "top" in faces and "bottom" in faces:
        t_top = loader.load_block(faces["top"])
        t_bot = loader.load_block(faces["bottom"])
        if t_top is not None and t_bot is not None:
            t_top = _apply_tint_if_gray(t_top, tint)
            t_bot = _apply_tint_if_gray(t_bot, tint)
            cw = max(t_top.width, t_bot.width)
            ch = t_top.height + t_bot.height
            comp = Image.new("RGBA", (cw, ch), (0, 0, 0, 0))
            comp.paste(t_top, ((cw - t_top.width) // 2, 0), t_top)
            comp.paste(t_bot, ((cw - t_bot.width) // 2, t_top.height), t_bot)
            return _square_pad_resize(comp, size)
    tname = faces.get("front") or faces.get("all") or faces.get("side") or faces.get("top") or name
    t = loader.load_block(tname) or loader.load_item(tname)
    if t is None:
        return None
    if tint is not None and tname in (faces.get("all"), faces.get("top")):
        t = _apply_tint_if_gray(t, tint)
    return _square_pad_resize(t, size)


SKIP_DISCOVER_PREFIXES = (
    "destroy_stage_x",
    "empty_armor_slot_",
)
SKIP_DISCOVER_EXACT = {
    "missing_tile",
    "fireworks_charge_overlay",
    "potion_overlay",
    "redstone_dust_cross_overlay",
    "redstone_dust_line_overlay",
} | {f"{_bk}_{_i}" for _bk in BED_KEYS for _i in range(1, 6)}


def _infer_block_spec(key: str) -> dict:
    cross_prefixes = (
        "flower_",
        "sapling",
        "tallgrass",
        "mushroom_red",
        "mushroom_brown",
        "wheat_stage",
        "carrots_stage",
        "potatoes_stage",
        "beetroot_stage",
        "nether_wart_stage",
        "cocoa_stage",
        "melon_stem",
        "pumpkin_stem",
        "sweet_berry_bush",
        "torch_",
        "redstone_torch_",
        "fire",
    )
    cross_exact = {"web", "reeds", "deadbush", "brewing_stand"}
    carpet_prefixes = ("rail_", "repeater_", "redstone_dust_")
    carpet_exact = {"waterlily", "itemframe_background", "trip_wire"}
    plate_exact = {"snow", "brewing_stand_base"}
    pane_exact = {"iron_bars", "copper_bars", "glass_pane_top", "glass_pane_top_shared", "ladder", "vine"}

    tint = GREEN_TINT if (key.startswith("leaves") or key.startswith("tallgrass") or key in ("vine", "waterlily")) else None

    if key.startswith(cross_prefixes) or key in cross_exact:
        spec: dict = {"kind": "block", "shape": "cross", "faces": {"all": key}}
    elif "trapdoor" in key:
        spec = {"kind": "block", "shape": "plate", "faces": {"all": key}}
    elif key in plate_exact:
        spec = {"kind": "block", "shape": "plate", "faces": {"all": key}}
    elif key.startswith(carpet_prefixes) or key in carpet_exact:
        spec = {"kind": "block", "shape": "carpet", "faces": {"all": key}}
    elif key in pane_exact or ("door" in key and key.endswith(("_lower", "_upper", "_bottom", "_top"))):
        spec = {"kind": "block", "shape": "pane", "faces": {"all": key}}
    else:
        spec = {"kind": "block", "shape": "cube", "faces": {"all": key}}

    if tint is not None and key != "tallgrass_1":
        spec["tint"] = tint
    return spec


def discover_all_entries(loader: AssetLoader, existing: Dict[str, dict]) -> Dict[str, dict]:
    out = dict(existing)

    block_keys = set(loader.terrain_meta.keys())
    for d in SEARCH_BLOCK_DIRS:
        if d.exists():
            for p in d.glob("*.png"):
                block_keys.add(p.stem)

    for k in sorted(block_keys):
        if k in out or k in SKIP_DISCOVER_EXACT or k.startswith(SKIP_DISCOVER_PREFIXES):
            continue
        out[k] = _infer_block_spec(k)

    item_keys = set(loader.items_meta.keys())
    for d in SEARCH_ITEM_DIRS:
        if d.exists():
            for p in d.glob("*.png"):
                item_keys.add(p.stem)

    for k in sorted(item_keys):
        if k in out or k in SKIP_DISCOVER_EXACT or k.startswith(SKIP_DISCOVER_PREFIXES):
            continue
        out[k] = {"kind": "item", "item_tex": k}

    return out


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate 2D and 3D PNG renders for all New Additions and game blocks/items.")
    parser.add_argument("--mode", choices=("2d", "3d", "both"), default="both")
    parser.add_argument("--size", type=int, default=256)
    parser.add_argument("--out", type=Path, default=REPO_ROOT / "wiki" / "renders")
    parser.add_argument("--only", nargs="*", default=None, help="Specific catalog or texture keys to render.")
    parser.add_argument("--all", action="store_true", help="Render all discovered game blocks, variants, and items (default).")
    parser.add_argument("--catalog-only", action="store_true", help="Restrict discovery to curated CATALOG entries only.")
    parser.add_argument("--list", action="store_true", help="List available keys and exit.")
    args = parser.parse_args()

    loader = AssetLoader()
    entries = dict(CATALOG) if args.catalog_only else discover_all_entries(loader, CATALOG)
    if args.list:
        for k in sorted(entries):
            print(f"{k}\t({entries[k]['kind']})")
        return 0

    targets = args.only if args.only else sorted(entries)

    if args.mode in ("2d", "both"):
        (args.out / "2d").mkdir(parents=True, exist_ok=True)
    if args.mode in ("3d", "both"):
        (args.out / "3d").mkdir(parents=True, exist_ok=True)

    rendered = 0
    for key in targets:
        spec = entries.get(key)
        if spec is None:
            if loader.load_block(key) is not None:
                spec = _infer_block_spec(key)
            elif loader.load_item(key) is not None:
                spec = {"kind": "item", "item_tex": key}
            else:
                print(f"[skip] unknown key: {key}", file=sys.stderr)
                continue

        if args.mode in ("2d", "both"):
            img2d = render_2d_asset(loader, key, spec, args.size)
            if img2d is not None:
                img2d.save(args.out / "2d" / f"{key}.png")
                rendered += 1

        if args.mode in ("3d", "both"):
            if spec["kind"] in ("block", "head"):
                img3d = render_3d_block(loader, spec, args.size)
            else:
                itex = loader.load_item(spec["item_tex"])
                img3d = render_3d_extruded_item(itex, args.size) if itex is not None else None
            if img3d is not None:
                img3d.save(args.out / "3d" / f"{key}.png")
                rendered += 1

    print(f"Generated {rendered} image(s) in {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

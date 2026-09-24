"""Run with UnrealEditor -ExecutePythonScript=<absolute script path>.

Read-only review of the existing studio: asset/actor inventory and eye-height
viewport captures. Does not save the level or change gameplay actors.
"""
import json
import time
from pathlib import Path
import unreal

output = Path(unreal.Paths.project_saved_dir()) / "VisualReview"
output.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level("/Game/TheFinalTake/Maps/L_FinalTake_Studio")
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
inventory = []
for actor in actors:
    meshes = []
    for component in actor.get_components_by_class(unreal.StaticMeshComponent):
        mesh = component.get_editor_property("static_mesh")
        meshes.append({
            "component": component.get_name(),
            "mesh": mesh.get_path_name() if mesh else None,
            "materials": [m.get_path_name() if m else None for m in component.get_materials()],
            "collision": str(component.get_collision_enabled()),
        })
    inventory.append({"actor": actor.get_actor_label(), "class": actor.get_class().get_name(), "meshes": meshes})
(output / "inventory.json").write_text(json.dumps(inventory, indent=2), encoding="utf-8")
materials = {}
for asset_path in unreal.EditorAssetLibrary.list_assets("/Game/TheFinalTake/Materials"):
    material = unreal.load_asset(asset_path)
    if not isinstance(material, unreal.Material):
        continue
    nodes = []
    for expression in unreal.MaterialEditingLibrary.get_material_expressions(material):
        node = {"class": expression.get_class().get_name(), "name": expression.get_name()}
        for key in ("parameter_name", "default_value", "constant", "code", "const_a", "const_b"):
            try:
                node[key] = str(expression.get_editor_property(key))
            except Exception:
                pass
        nodes.append(node)
    materials[asset_path] = nodes
(output / "materials.json").write_text(json.dumps(materials, indent=2), encoding="utf-8")
levels.editor_set_game_view(True)
views = [
    ("01_street", (-2850, -650, 170), (5, 22, 0)),
    ("02_lobby", (-1560, -240, 162), (0, 12, 0)),
    ("03_office", (-1460, 1170, 162), (-8, 40, 0)),
    ("04_wardrobe", (-1470, -1110, 162), (-4, -35, 0)),
    ("05_stage", (150, -450, 162), (0, 20, 0)),
    ("06_tank", (1000, 0, 162), (-3, 0, 0)),
    ("07_warehouse", (1650, 1440, 42), (0, 50, 0)),
    ("08_projection", (-470, -1470, 582), (0, 20, 0)),
    ("09_crew", (-1420, 0, 145), (-7, 0, 0)),
    ("10_costumes", (-1420, 0, 145), (-7, 0, 0)),
]
if "-FTReviewModelsOnly" in unreal.SystemLibrary.get_command_line():
    views = [view for view in views if view[0] in ("06_tank", "09_crew", "10_costumes")]
elif "-FTReviewRoomsOnly" in unreal.SystemLibrary.get_command_line():
    views = [view for view in views if view[0] in ("04_wardrobe", "07_warehouse", "08_projection")]
state = {"index": 0, "phase": "move", "at": time.monotonic() + 15, "task": None}
review_crew = []

def review_tick(delta):
    if time.monotonic() < state["at"]:
        return
    if state["index"] >= len(views):
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.log("FT_VISUAL_REVIEW_COMPLETE")
        unreal.SystemLibrary.quit_editor()
        return
    name, position, rotation = views[state["index"]]
    if state["phase"] == "move":
        if name == "09_crew":
            actor_tools = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
            character_class = unreal.load_class(None, "/Script/The_Final_Take.FTCharacter")
            for index in range(4):
                character = actor_tools.spawn_actor_from_class(character_class, unreal.Vector(-950, -180 + index * 120, 88), unreal.Rotator(yaw=180))
                character.set_editor_property("crew_index", index)
                character.call_method("OnRep_Look")
                review_crew.append(character)
        elif name == "10_costumes":
            for character, costume in zip(review_crew, (unreal.FTCostume.LIFEGUARD, unreal.FTCostume.SHARK, unreal.FTCostume.RAINCOAT, unreal.FTCostume.FOAM_KNIGHT)):
                character.set_editor_property("costume", costume)
                character.call_method("OnRep_Look")
        editor.set_level_viewport_camera_info(unreal.Vector(*position), unreal.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
        state.update(phase="capture", at=time.monotonic() + 8)
    elif state["phase"] == "capture":
        state["task"] = unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, str(output / (name + ".png")))
        state.update(phase="wait", at=time.monotonic() + 3)
    else:
        if state["task"] and not state["task"].is_task_done():
            return
        state.update(index=state["index"] + 1, phase="move", at=time.monotonic() + 1)

handle = unreal.register_slate_post_tick_callback(review_tick)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)

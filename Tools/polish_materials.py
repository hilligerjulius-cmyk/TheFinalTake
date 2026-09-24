"""Polish the existing studio materials in Unreal; preserve all colour/emissive inputs.

Run with UnrealEditor-Cmd <project> -run=pythonscript -script=<this file>
The generated nodes carry descriptions so repeating this script is idempotent.
"""
import unreal

edit = unreal.MaterialEditingLibrary

def node(material, cls, label, x, y):
    for existing in edit.get_material_expressions(material):
        if existing.get_editor_property("desc") == label:
            return existing, False
    expression = edit.create_material_expression(material, cls, x, y)
    expression.set_editor_property("desc", label)
    return expression, True

for name in ("M_FT_Matte", "M_FT_MatteISM"):
    material = unreal.load_asset("/Game/TheFinalTake/Materials/" + name)
    assert material, name
    original_color = edit.get_material_property_input_node(material, unreal.MaterialProperty.MP_BASE_COLOR)
    tint, created = node(material, unreal.MaterialExpressionMultiply, "FT finish: subtle pigment variation", 900, 0)
    if created:
        assert original_color, "Base colour input missing"
        position, _ = node(material, unreal.MaterialExpressionWorldPosition, "FT finish: world position", 0, 600)
        variation, _ = node(material, unreal.MaterialExpressionCustom, "FT finish: pigment", 300, 600)
        variation.set_editor_property("code", "return 1.0 + 0.018 * sin(P.x * 0.035 + sin(P.y * 0.019)) * sin(P.z * 0.027 + P.y * 0.013);")
        variation.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
        custom_input = unreal.CustomInput()
        custom_input.set_editor_property("input_name", "P")
        variation.set_editor_property("inputs", [custom_input])
        assert edit.connect_material_expressions(position, "", variation, "P")
        assert edit.connect_material_expressions(original_color, "", tint, "A")
        assert edit.connect_material_expressions(variation, "", tint, "B")
        assert edit.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough, _ = node(material, unreal.MaterialExpressionLinearInterpolate, "FT finish: surface roughness", 900, 300)
    rough.set_editor_property("const_a", 0.78)
    rough.set_editor_property("const_b", 0.24)
    if name.endswith("ISM"):
        gloss, _ = node(material, unreal.MaterialExpressionPerInstanceCustomData, "FT finish: instance gloss", 600, 350)
        gloss.set_editor_property("data_index", 4)
        gloss.set_editor_property("const_default_value", 0.0)
    else:
        gloss = next(e for e in edit.get_material_expressions(material)
                     if isinstance(e, unreal.MaterialExpressionScalarParameter)
                     and str(e.get_editor_property("parameter_name")) == "Gloss")
    assert edit.connect_material_expressions(gloss, "", rough, "Alpha")
    assert edit.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    edit.recompile_material(material)
    assert unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.log("FT_FINISH_SAVED " + name)

outline = unreal.load_asset("/Game/TheFinalTake/Materials/PP_FT_Outline")
for expression in edit.get_material_expressions(outline):
    if expression.get_name() == "MaterialExpressionMultiply_0":
        expression.set_editor_property("const_b", 0.85)
    elif expression.get_name() == "MaterialExpressionMultiply_9":
        expression.set_editor_property("const_b", 0.26)
edit.recompile_material(outline)
assert unreal.EditorAssetLibrary.save_loaded_asset(outline)
unreal.log("FT_FINISH_COMPLETE")

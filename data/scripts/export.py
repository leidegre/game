import bpy
import sys
import os
import struct
import array

# "C:\Program Files\Blender Foundation\Blender 2.93\blender.exe" data\vehicles\platforms\hierarchy.blend --background -P data\scripts\export.py -- foo

print(sys.argv)
print(bpy.data.filepath)

file_root, file_ext = os.path.splitext(bpy.data.filepath)
file_mesh = file_root + ".vdf"  # Vertex Data Format

print(file_mesh)

object_name = "platform1"
obj = bpy.data.objects.get(object_name)

if obj and obj.type == 'MESH':
    mesh = obj.data

    # Ensure you're in Object mode
    bpy.ops.object.mode_set(mode='OBJECT')

    # Set as active object
    bpy.context.view_layer.objects.active = obj

    # Apply all modifiers
    bpy.ops.object.convert(target='MESH')

    # Open output file
    with open(file_mesh, 'wb') as f:
        # MagicNumber           4 bytes ASCII
        # VertexFormat          4 bytes uint32 1 = 3 component 32-bit floating point data
        # VertexCount           4 bytes uint32
        # OffsetToPositionData  4 bytes uint32
        # OffsetToNormalData    4 bytes uint32
        # OffsetToColorData     4 bytes uint32
        # TriangleCount         4 bytes uint32
        # OffsetToTriangleData  4 bytes uint32

        # Reserve space for header
        f.write(b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00")
        f.write(b"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00")

        # Get vertex coordinates and normals
        vertices = [v.co for v in mesh.vertices]
        vertex_array = array.array(
            'f', [coord for vertex in vertices for coord in vertex])

        normals = [v.normal for v in mesh.vertices]
        normal_array = array.array(
            'f', [coord for vertex in normals for coord in vertex])

        # Get triangle indices
        triangles = [list(p.vertices)
                     for p in mesh.polygons if len(p.vertices) == 3]
        triangles_array = array.array(
            'H', [coord for vertex in triangles for coord in vertex])  # uint16

        # Get vertex colors
        if len(mesh.vertex_colors) > 0:
            color_layer = mesh.vertex_colors.active.data
            vertex_colors = [
                color_layer[i].color for i in range(len(color_layer))]
        else:
            vertex_colors = []

        # Position data
        offset_to_vertex_data = f.tell()
        vertex_array.tofile(f)

        offset_to_normals_data = f.tell()
        normal_array.tofile(f)

        offset_to_triangle_data = f.tell()
        triangles_array.tofile(f)

        # Write header
        f.seek(0)
        f.write(struct.pack('4sIIIIIII', b"VDF1",
                            1, len(vertices), offset_to_vertex_data, offset_to_normals_data, 0, len(triangles), offset_to_triangle_data))

        # Print data
        print(f"Vertices:  {len(vertices)}")
        print(f"Normals:   {len(normals)}")
        print(f"Triangles: {len(triangles)}")
else:
    print(f"No mesh object named {object_name} found.")

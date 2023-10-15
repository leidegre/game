# DX12 rendering backend

The rendering backend may access game state and components but not the other way around.
If I wanted to make a new DX12 backend I should be able to do so.
If I wanted to swap between different DX12 backends I should be able to do so.

# What does our rendering system look like?

First we do a culling pass to determine what to draw.

We're going to do vertex data only, i.e. position, color, normal. that's it. We're not going to be using texture mapping or texture data. Still going for PBR but we aren't going to do per pixel lighting with a lot of texture data.

# Modelling

I want you to be able to build your own units. To do this you need a platform. On top of this platform you can mount different "things". A typical car platform is a base with 4 wheels attached to it. I quick image search for "vehicle platform" will give you an idea what this might look like. We can then consider what housing to put on top and what kind of mount points different housings have for utilities. Like grapplers and what not.

Gotta keep things simple to begin with. These modular components that we then render into assets must be exportable and workable from blender. So we need to basically setup a script to process our blender file and manage the asset data. If we want it "hot reload" we need to take care of how we put the asset into the GPU memory and how we reference it so. There's that.

Could toy a bit more with this.

Should play more with Blender to see how these things could work in practice. Then gotta figure out how to do animation.

Parts can be independently animated. Like treads, tracks. It's rotation and a chain that is moving around the lines. The animation is just treads following a line with some initial offset. So that could be useful.

Platforms can have various mount points which control what parts are compatible. All of this could probably be prototyped in blender. Then we figure out how we drive it inside our engine.

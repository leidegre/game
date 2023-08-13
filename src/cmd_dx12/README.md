This is just a sample application to initialize DirectX 12 and draw a basic triangle in the viewport.

# How this came about

Step 1. Write a basic DX12 application. Learn how to setup and synchronize with the GPU.

Step 2. Thinking at a higher level. What is it that we want to accomplish here. We want to create a very basic renderer. To start, it will need to draw trivial triangle based meshes with vertex coloring.

# Designing a DX12 renderer

You need to take a part the rendering process into separate stages.

It's not uncommon for example, to have an initial depth pre-pass. At this stage we're just rendering depth information into the depth buffer.

Whatever the requirements for this stage is, it will show in our root signatures and pipeline state object.

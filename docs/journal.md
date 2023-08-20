# 2023-08-19

Using colum-major matrices because it is the default HLSL convention is good.

Using aggregate initialization to construct matrices that use column-major layout directly is bad. It will result in code that doesn't match standard notation.

If the PSO BlendState is zero initialized pixel debugging "might" work but the output to the render target will be black.

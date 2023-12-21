---
layout: default
title: Geometry Extrusion
parent: Miscellaneous
nav_order: 2
---

# Extrude a Geometry from 2D to 3D
{: .fs-9 .fw-500}

---

Extrude a 2D domain into 3D, allowing users to define the extrusion direction and extent. Additionally, users have the flexibility to specify the boundary conditions for the extruded domain.

__Example usage:__

<details markdown="1">
<summary markdown="1">
C++
{: .label .label-blue}
</summary>
```c++
psExtrude<double>(domain2D, domain3D, 
                  {0., 1.}, // min and max extent in the extruded dim
                  2, // extrude in z-direction
                  {lsBoundaryConditionEnum::REFLECTIVE,
                   lsBoundaryConditionEnum::REFLECTIVE,
                   lsBoundaryConditionEnum::INFINITE_BOUNDARY}).apply();
```
</details>

{: .note}
> Since the ViennaPS 2D and 3D libraries cannot be used together in Python, this feature is currently not available in the Python bindings.
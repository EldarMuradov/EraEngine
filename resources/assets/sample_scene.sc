Scene: My scene
Camera:
  Rotation: [0.284642398, 0.464300722, -0.160547212, 0.823182344]
  Position: [1.81936193, 3.60305071, 0.501671314]
  Near plane: 0.200000003
  Far plane: -1
  Type: 0
  Vertical FOV: 1.22173047
  Fx: 0
  Fy: 0
  Cx: 0
  Cy: 0
Rendering:
  Tonemap:
    A: 0.191
    B: 0.0820000023
    C: 1
    D: 0.5
    E: 0.0820000023
    F: 0.254999995
    Linear white: 11.1999998
    Exposure: -0.0599999987
  Enable AO: true
  AO:
    Threshold: 1
    Num rays: 4
    Max num steps per ray: 10
    Strength: 1
  Enable SSS: true
  SSS:
    Num steps: 16
    Ray distance: 0.5
    Thickness: 0.0500000007
    Max distance from camera: 5
    Distance fadeout range: 2
    Border fadeout: 0.100000001
  Enable SSR: true
  SSR:
    Num steps: 400
    Max distance: 1000
    Stride cutoff: 100
    Min stride: 1
    Max stride: 30
  Enable TAA: false
  TAA:
    Camera jitter strength: 1
  Enable Bloom: true
  Bloom:
    Threshold: 70
    Strength: 0.5
  Enable sharpen: true
  Sharpen:
    Strength: 0.100000001
Sun:
  Color: [1, 0.930000007, 0.75999999]
  Intensity: 11.1000004
  Direction: [-0.498272866, -0.830454767, -0.249136433]
  Cascades: 3
  Cascade distances: [9, 25, 50, 10000]
  Bias: [0.000587999995, 0.000783999974, 0.000823999988, 0.00350000011]
  Blend distances: [5, 10, 10, 10]
  Shadow dimensions: 2048
  Stabilize: true
Environment:
  Type: Procedural
  Sun direction: [-0.498272866, -0.830454767, -0.249136433]
  GI mode: 0
  GI intensity: 1
  Sky intensity: 1
Entities:
  - Tag: Platform
    Transform:
      Position: [10, -4, 0]
      Rotation: [0, 0, 0, 1]
      Scale: [1, 1, 1]
    Mesh:
      Handle: 0
      Flags: 0
    Colliders:
      - Type: AABB
        Min corner: [-30, -4, -30]
        Max corner: [30, 4, 30]
        Restitution: 0.100000001
        Friction: 1
        Density: 4
  - Tag: Trigger
    Colliders:
      - Type: AABB
        Min corner: [20, 0, -10]
        Max corner: [30, 2, 0]
        Restitution: 0
        Friction: 0
        Density: 0
  - Tag: PlanePX
    Transform:
      Position: [0, -5, 0]
      Rotation: [0, 0, 0, 1]
      Scale: [1, 1, 1]
    Dynamic: true
  - Tag: CharacterControllerPx
    Transform:
      Position: [20, 5, -5]
      Rotation: [0, 0, 0, 0.999961913]
      Scale: [1, 1, 1]
    Dynamic: true
  - Tag: SpherePX1
    Transform:
      Position: [20, 36, -5]
      Rotation: [0, 0, 0, 0.999961913]
      Scale: [1, 1, 1]
    Dynamic: true
    Mesh:
      Handle: 0
      Flags: 0
  - Tag: SpherePX
    Transform:
      Position: [20, 30, -5]
      Rotation: [0, 0, 0, 0.999961913]
      Scale: [1, 1, 1]
    Dynamic: true
    Mesh:
      Handle: 0
      Flags: 0
  - Tag: Sphere
    Transform:
      Position: [25, 16, -5]
      Rotation: [0, 0, 0.00872653536, 0.999961913]
      Scale: [1, 1, 1]
    Dynamic: true
    Mesh:
      Handle: 0
      Flags: 0
    Rigid body:
      Local COG: [0, 0, 0]
      Inv mass: 0.238732412
      Inv inertia: [0.596831024, 0, 0, 0, 0.596831024, 0, 0, 0, 0.596831024]
      Gravity factor: 1
      Linear damping: 0.400000006
      Angular damping: 0.400000006
    Colliders:
      - Type: Sphere
        Center: [0, 0, 0]
        Radius: 1
        Restitution: 0.100000001
        Friction: 0.5
        Density: 1
  - Tag: Sphere
    Transform:
      Position: [25, 13, -5]
      Rotation: [0, 0, 0.00872653536, 0.999961913]
      Scale: [1, 1, 1]
    Dynamic: true
    Mesh:
      Handle: 0
      Flags: 0
    Rigid body:
      Local COG: [0, 0, 0]
      Inv mass: 0.238732412
      Inv inertia: [0.596831024, 0, 0, 0, 0.596831024, 0, 0, 0, 0.596831024]
      Gravity factor: 1
      Linear damping: 0.400000006
      Angular damping: 0.400000006
    Colliders:
      - Type: Sphere
        Center: [0, 0, 0]
        Radius: 1
        Restitution: 0.100000001
        Friction: 0.5
        Density: 1
  - Tag: Sphere
    Transform:
      Position: [25, 10, -5]
      Rotation: [0, 0, 0.00872653536, 0.999961913]
      Scale: [1, 1, 1]
    Dynamic: true
    Mesh:
      Handle: 0
      Flags: 0
    Rigid body:
      Local COG: [0, 0, 0]
      Inv mass: 0.238732412
      Inv inertia: [0.596831024, 0, 0, 0, 0.596831024, 0, 0, 0, 0.596831024]
      Gravity factor: 1
      Linear damping: 0.400000006
      Angular damping: 0.400000006
    Colliders:
      - Type: Sphere
        Center: [0, 0, 0]
        Radius: 1
        Restitution: 0.100000001
        Friction: 0.5
        Density: 1
  - Tag: Sponza
    Transform:
      Position: [0, 1, 0]
      Rotation: [0, 0, 0, 1]
      Scale: [0.00999999978, 0.00999999978, 0.00999999978]
    Mesh:
      Handle: 0
      Flags: 15
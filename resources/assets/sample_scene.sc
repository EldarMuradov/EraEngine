Scene: My scene
Camera:
  Rotation: [0.0288985185, -0.714919269, 0.0295998193, 0.697982013]
  Position: [-22.4972916, 5.5241127, 5.58542871]
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
    A: 0.340000004
    B: 0.150000006
    C: 0.0769999996
    D: 0.375
    E: 0.00999999978
    F: 0.340000004
    Linear white: 11.1999998
    Exposure: -0.0799999982
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
  Enable TAA: true
  TAA:
    Camera jitter strength: 1
  Enable Bloom: true
  Bloom:
    Threshold: 0.5
    Strength: 0.449999988
  Enable sharpen: true
  Sharpen:
    Strength: 0.230000004
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
  GI intensity: 0.340000004
  Sky intensity: 1.85000002
Entities:
  - Tag: SpherePX
    Transform:
      Position: [-10, 5, -3]
      Rotation: [0, 0, 0, 0.999961913]
      Scale: [1, 1, 1]
    Mesh:
      Handle: 0
      Flags: 0
  - Tag: SpherePX1
    Transform:
      Position: [5, 155, 5]
      Rotation: [0, 0, 0, 0.999961913]
      Scale: [5, 5, 5]
    Mesh:
      Handle: 0
      Flags: 0
  - Tag: Platform
    Transform:
      Position: [10, -9, 0]
      Rotation: [0, 0, 0, 1]
      Scale: [5, 1, 5]
    Mesh:
      Handle: 0
      Flags: 0
  - Tag: Sponza
    Transform:
      Position: [5, -3.75, 35]
      Rotation: [0, 0, 0, 1]
      Scale: [0.00999999978, 0.00999999978, 0.00999999978]
    Mesh:
      Handle: 11742736751713587695
      Flags: 15
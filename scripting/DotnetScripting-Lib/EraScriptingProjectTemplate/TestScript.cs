using EraEngine.Core;
using EraEngine;
using EraEngine.Components;
using System.Numerics;

namespace EraScriptingProjectTemplate
{
    public class TestScript : Script
    {
        private RigidbodyComponent _rb;
        public override void Start()
        {
            _rb = Entity.CreateComponentInternal<RigidbodyComponent>(RigidbodyType.Dynamic);
        }

        public override void Update(float dt)
        {
            _rb.AddForce(new Vector3(0, 10, 0), ForceMode.Impulse);
        }
    }
}

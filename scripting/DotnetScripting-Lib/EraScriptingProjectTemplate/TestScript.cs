using EraEngine.Core;
using EraEngine;
using EraEngine.Components;
using System.Numerics;
using EraEngine.AI;

namespace EraScriptingProjectTemplate
{
    public class TestScript : Script
    {
        private TransformComponent _target;
        private NavigationComponent _nav_comp;

        public override void Start()
        {
            _nav_comp = Entity.CreateComponentInternal<NavigationComponent>(NavigationType.A_Star);
            _target = Scene.GetEntity(3).GetComponent<TransformComponent>();
        }

        public override void Update(float dt)
        {
            _nav_comp.SetDestination(_target.GetPosition());
        }
    }
}

using EraScriptingCore.Core;
using EraScriptingCore.Domain;
using EraScriptingCore.Domain.Components;
using System.Numerics;

namespace EraScriptingProjectTemplate
{
    public class TestScript : Script
    {
        public override void Start()
        {
            Console.WriteLine("User script");
            Entity.CreateComponentInternal<RigidbodyComponent>(RigidbodyType.Dynamic).AddForce(new System.Numerics.Vector3(0, 10, 0), ForceMode.Impulse);

            Entity.GetComponent<RigidbodyComponent>().Mass = 5.0f;

            float mass = Entity.GetComponent<RigidbodyComponent>().Mass;
            Debug.Log($"Mass = {mass}");
        }

        public override void Update(float dt)
        {
            Vector3 velocity = Entity.GetComponent<RigidbodyComponent>().GetLinearVelocity();
            Debug.Log($"X = {velocity.X}, Y = {velocity.Y}, Z = {velocity.Z}");
        }
    }
}

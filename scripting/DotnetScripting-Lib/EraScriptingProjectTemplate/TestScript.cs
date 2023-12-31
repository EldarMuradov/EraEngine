using EraEngine.Core;
using EraEngine;
using EraEngine.Components;
using System.Numerics;

namespace EraScriptingProjectTemplate
{
    public class TestScript : Script
    {
        public override void Start()
        {
            Console.WriteLine("User script");
            Entity.CreateComponentInternal<RigidbodyComponent>(RigidbodyType.Dynamic).AddForce(new Vector3(0, 10, 0), ForceMode.Impulse);

            Entity.GetComponent<RigidbodyComponent>().Mass = 5.0f;

            float mass = Entity.GetComponent<RigidbodyComponent>().Mass;
            Debug.Log($"Mass = {mass}");
        }

        public override void Update(float dt)
        {
            //Vector3 velocity = Entity.GetComponent<RigidbodyComponent>().GetLinearVelocity();
            //Debug.Log($"X = {velocity.X}, Y = {velocity.Y}, Z = {velocity.Z}");

            Vector3 pos = Entity.GetComponent<TransformComponent>().GetPosition();
            Debug.Log($"Position X = {pos.X}, Y = {pos.Y}, Z = {pos.Z}");


            Quaternion rot = Entity.GetComponent<TransformComponent>().GetRotation();
            Debug.Log($"Rotation X = {rot.X}, Y = {rot.Y}, Z = {rot.Z}, W = {rot.W}");
        }
    }
}

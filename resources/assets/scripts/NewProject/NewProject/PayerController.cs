#pragma warning disable CS8618 
#pragma warning disable CS8601 

using EraEngine;
using EraEngine.Components;
using System.Numerics;

namespace NewProject;

public sealed class PlayerController : EScript
{
    private RigidbodyComponent _rigidbody;

    private WeaponHandler _weaponHandler;

    [HideInInspector]
    private PlayerInputManager _playerInputManager;

    private bool _isGrounded = false;

    private float _jumpStrength = 2.0f;

    private float _hp = 100;

    public float Hp 
    {
        get => _hp;
        set 
        {
            if (value <= 0.0f)
                Die();
            _hp = value;
        }
    }

    public override void Start()
    {
        _rigidbody = Entity.CreateComponent<RigidbodyComponent>();
        _weaponHandler = Entity.CreateComponent<WeaponHandler>();
        _playerInputManager = Entity.CreateComponent<PlayerInputManager>();

        Debug.Log("Player controller started");
    }

    public override void Update(float dt)
    {
        _playerInputManager.ProcessInput(dt);
    }

    public override void OnTriggerEnter(EEntity trigger)
    {
        if (trigger.GetComponent<Bullet>() is not null)
            Hp -= 10.0f;
    }

    public override void OnCollisionEnter(EEntity collision)
    {
        var pc = collision.GetComponent<PlayerController>();
        if (pc is null)
            return;

        // TODO: Add more complex logic
        Debug.Log("Hit!");
    }

    public void Jump()
    {
        if (_isGrounded)
            _rigidbody.AddForce(new Vector3(0, 1, 0) * _jumpStrength, ForceMode.Impulse);
    }

    private void Die() 
    {
        Debug.Log("You died!");
    }
}






































#pragma warning restore CS8618
#pragma warning restore CS8601
# Quick Start
Evaluate [Rego](https://www.openpolicyagent.org/docs/latest/policy-language/) natively
in .NET programs easily using this library.

## Creating a Project

In this section we will write a policy class that uses Rego to determine room
access in a building.

> Prerequisites
> * Install [.NET SDK](https://dotnet.microsoft.com/en-us/download) 8.0 or higher

Make sure you have [.NET SDK](https://dotnet.microsoft.com/en-us/download)
installed, then open a terminal and enter the following commands to create a new
project and install the latest version of Rego:

    dotnet new console -n MyPolicy
    cd MyPolicy
    dotnet add package Rego --version 1.1.0

## Adding a Policy Class

Begin by added the following C# class to your new project:

`RoomAccess.cs`
```csharp
using Rego;

class RoomAccess
{
    /// <summary>
    /// This is a Rego policy for room access
    /// </summary>
    const string Policy = """
    package room

    default allow := false

    allow if {
        input.room in data.users[input.name].clearance
    }

    allow if {
        data.users[input.name].security_staff
        input.room in data.rooms
    }

    errors contains "unknown room" if {
        not input.room in data.rooms
    }

    errors contains "unknown user" if {
        not data.users[input.name]
    }

    errors contains "user does not have access to room" if {
        user := data.users[input.name]
        not user.security_staff
        input.room in data.rooms
        not input.room in user.rooms
    }
    """;

    /// <summary>
    /// The interpreter object applies policy to inputs
    /// </summary>
    private Interpreter m_interpreter;

    /// <summary>
    /// A bundle is a compiled policy that can be reused with different inputs
    /// </summary>
    private Bundle m_bundle;

    /// <summary>
    /// The constructor takes a JSON string representing the room access database. This
    /// will contain a list of user objects specifying clearance or security staff status,
    /// and a list of rooms.
    /// </summary>
    /// <param name="database_json">JSON database</param>
    public RoomAccess(string database_json)
    {
        m_interpreter = new Interpreter();
        m_interpreter.AddModule("room.rego", Policy);
        m_interpreter.AddDataJson(database_json);

        // when compilng this policy, we will specify two entrypoints for later evaluation
        m_bundle = m_interpreter.Build(["room/allow", "room/errors"]);
    }

    /// <summary>
    /// Checks whether `name` can access `room`.
    /// </summary>
    /// <param name="name">The user name</param>
    /// <param name="room">The room ID</param>
    /// <returns>Whether the user is allowed to access the room</returns>
    public bool Check(string name, string room)
    {
        // First, we set the input of the interpreter with the new data
        m_interpreter.SetInput(Input.Create(new Dictionary<string, object>
        {
            {"name", name },
            {"room", room }
        }));

        // Now we query the bundle.
        var output = m_interpreter.QueryBundle(m_bundle, "room/allow");
        if ((bool)output.Expressions()[0].Value)
        {
            // Access is granted
            Console.WriteLine("{0} is allowed in room {1}", name, room);
            return true;
        }

        // The user was denied, so we query the policy to find out why
        Console.WriteLine("{0} is denied access to room {1}", name, room);
        output = m_interpreter.QueryBundle(m_bundle, "room/errors");
        Console.Error.WriteLine(output);
        return false;
    }
}
```

This class will allow us to apply Rego policy to inputs.

## Putting it All Together

Now that we have a policy class, we can exercise it to see how it performs.
Add the following file to your project:

`Program.cs`
```csharp
RoomAccess roomAccess = new RoomAccess("""
{
    "users": {
        "alice": {
            "clearance": ["alpha", "gamma"]
        },
        "bob": {
            "clearance": ["beta", "gamma"]
        },
        "charlie": {
            "security_staff": true
        }
    },
    "rooms": ["alpha", "beta", "gamma"]
}
""");

roomAccess.Check("alice", "alpha");
// alice is allowed in room alpha
roomAccess.Check("bob", "alpha");
// bob is denied access to room alpha
// {"expressions":[["user does not have access to room"]]}
roomAccess.Check("charlie", "gamma");
// charlie is allowed in room gamma
roomAccess.Check("doug", "alpha");
// doug is denied access to room alpha
// {"expressions":[["unknown user"]]}
roomAccess.Check("charlie", "epsilon");
// charlie is denied access to room epsilon
// {"expressions":[["unknown room"]]}
```

Now we can run our program:

    dotnet run

The output should be:

```bash
alice is allowed in room alpha

bob is denied access to room alpha
{"expressions":[["user does not have access to room"]]}

charlie is allowed in room gamma

doug is denied access to room alpha
{"expressions":[["unknown user"]]}

charlie is denied access to room epsilon
{"expressions":[["unknown room"]]}
```

## Next Steps

- [Getting Started](docs/getting-started.md)
- [Interpreter](xref:Rego.Interpreter?title=Interpreter+Class)
- [API](xref:Rego?title=API)

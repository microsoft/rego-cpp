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
            Console.WriteLine("{0} is allowed in room {1}\n", name, room);
            return true;
        }

        // The user was denied, so we query the policy to find out why
        Console.WriteLine("{0} is denied access to room {1}", name, room);
        output = m_interpreter.QueryBundle(m_bundle, "room/errors");
        Console.Error.WriteLine(output);
        Console.WriteLine();
        return false;
    }
}

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
roomAccess.Check("bob", "alpha");
roomAccess.Check("charlie", "gamma");
roomAccess.Check("doug", "alpha");
roomAccess.Check("charlie", "epsilon");
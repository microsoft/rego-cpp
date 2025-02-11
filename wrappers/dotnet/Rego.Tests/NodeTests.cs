namespace Rego.Tests;

using Rego;

[Collection("RegoTests")]
public class NodeTests
{
    [Fact]
    public void Node_access()
    {
        var rego = new Interpreter();
        var result = rego.Query("x={\"a\": 10, \"b\": \"20\", \"c\": [30.0, 60], \"d\": true, \"e\": null}");
        Assert.NotNull(result);
        var x = result.Binding("x");
        Assert.NotNull(x);
        Assert.Equal("{\"a\":10, \"b\":\"20\", \"c\":[30, 60], \"d\":true, \"e\":null}", x.ToJson());
        Assert.Equal(10, x["a"].Value);
        Assert.Equal("\"20\"", x["b"].Value);
        Assert.Equal(30.0, x["c"][0].Value);
        Assert.Equal(60, x["c"][1].Value);
        Assert.True((bool)x["d"].Value);
        Assert.IsType<RegoNull>(x["e"].Value);
    }
}
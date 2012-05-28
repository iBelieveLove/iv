describe("Arguments", function() {
  it("duplicate parameter names", function() {
    function test(a, a) {
      expect(arguments[0]).toBe(0);
      expect(arguments[1]).toBe(1);
      a = 20;
      expect(arguments[0]).toBe(0);
      expect(arguments[1]).toBe(20);
    }
    test(0, 1);
  });
});
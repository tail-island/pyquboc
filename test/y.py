from cpp_pyquboc import Binary


x, y, z = Binary('x'), Binary('y'), Binary('z')

print((x * y * z).compile().to_bqm())
print((x + y + z).compile().to_bqm())
print((x * y + z).compile().to_bqm())
print(((x + y) * (x + y)).compile().to_bqm())
print(((x + y) * (x + y + z)).compile().to_bqm())
print((x * y * z * 2).compile().to_bqm())

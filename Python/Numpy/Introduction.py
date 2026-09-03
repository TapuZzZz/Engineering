import numpy as np

array_1d = np.array([1, 2, 3, 4, 5])
print("1D Array:")
print(array_1d)

array_2d = np.array([[1, 2, 3], [4, 5, 6]])
print("\n2D Array:")
print(array_2d)

print("\nShape of 2D Array:")
print(array_2d.shape)

print("Maximum value in 2D Array:")
print(np.max(array_2d))

print("Reshaped Array:")
reshaped_array = np.reshape(array_2d, (3, 2))
print(reshaped_array)

print("Shape of Reshaped Array:")
print(reshaped_array.shape)

print("Linspace Array:")
linspace_array = np.linspace(0, 1, 5)
print(linspace_array)

print("Zeros Array:")
print(np.zeros((3, 4)))

print("Ones Array:")
print(np.ones((2, 3), dtype=float))

print("Arange Array:")
arange_array = np.arange(0, 10, 2)
print(arange_array)

print("Minimum value in 2D Array:")
print(np.min(array_2d,axis=0))
print(np.min(array_2d,axis=1))

print("Median along columns:")
print(np.median(array_2d, axis=0))
print("Median along rows:")
print(np.median(array_2d, axis=1))

print("Standard deviation along columns:")
print(np.std(array_2d, axis=0))
print("Standard deviation along rows:")
print(np.std(array_2d, axis=1))

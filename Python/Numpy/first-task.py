import numpy as np

arr = np.array([1,2,3,4,5,6,7,8,9,10])
arr2 = np.ones(20)
arr3 = np.full(20, fill_value=4)
arr4 = np.arange(4, 84, 4)
rng = np.random.default_rng()
arr5 = rng.integers(1, 21, size=25)
arr5[:] = arr5 + 4
index = np.where(arr5 > 15)
print(arr5[index])
test1 = rng.integers(0, 101, size=40)
test1 = np.where(test1 > 90, 100, test1)
test1 = np.where(test1 < 55, 0, test1)
print(np.sum(test1))
test2 = rng.integers(0, 101, size=40)
test_avg = np.mean([test1, test2], axis=0)
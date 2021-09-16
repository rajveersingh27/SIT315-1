// Defines the function to be run on each of the devices
__kernel void execute(const int sizeTwo, const int sizeOne, __global int *v, __global int *v2,
                               __global int *a) {

  // Thread identifiers
  int row = get_global_id(0);
  int col = get_global_id(1);

  int sum = 0;
  for (int i = 0; i < sizeTwo; i++) {
    sum += v[row * sizeTwo + i] * v2[i * sizeOne + col];
  }

  a[row * sizeOne + col] = sum;
}
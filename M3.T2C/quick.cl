__kernel void quicksort(__global int *array, __global int *stack)
{
    const int chunk_size = get_global_id(0);
    int t;
    int min = 0;
    int max = chunk_size - 1;
    int top = -1;
    

    stack[++top] = min;
    stack[++top] = max;

    while (top >= 0)
    {
        max = stack[top--];
        min = stack[top--];

        int pivot = array[max];
        int index = min;

        for (int i = min; i < max; i++)
        {
            if (array[i] <= pivot)
            {
                t = array[i]; 
                array[i] = array[index]; 
                array[index] = t; 
                index++;
            }
        }

        t = array[index];
        array[index] = array[max];
        array[max] = t;
        if (index + 1 < max)
                {
                    stack[++top] = index + 1;
                    stack[++top] = max;
                }
        if (index - 1 > min)
        {
            stack[++top] = min;
            stack[++top] = index - 1;
        }
    }
}
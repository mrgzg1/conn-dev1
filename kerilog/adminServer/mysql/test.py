import random
import core

def test_insert_points(
            sense_id="asdfka",
            base_time=1533322985,
            num_test_points=100,
            min_test_data=0,
            max_test_data=100):
    data_list = []
    for each in range(num_test_points):
        data_list.append(
            [
                sense_id,
                base_time + (60*each),
                random.randint(min_test_data, max_test_data)
            ]
        )
    core.insert_data_points(data_list)

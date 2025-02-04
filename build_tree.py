# import piles
import argparse
import bitfiltertree

# Add argument parsing
parser = argparse.ArgumentParser(description='Build a bit filter tree')
parser.add_argument('--max-cube', type=int, required=True,
                   help='Maximum cube value')
parser.add_argument('--min-patterns-leaf', type=int, required=True,
                   help='Minimum number of patterns per leaf')

args = parser.parse_args()

# print(args.max_cube, args.min_patterns_leaf)
b = bitfiltertree.BitFilterTree(args.max_cube)
# b.read_filters_from_csv('diophantine.txt','diophantine.cache')
b.load_filters_from_cache('diophantine.cache')
b.build_tree_file(
    tree_path=f'filters/tree_{args.max_cube}_{args.min_patterns_leaf}.bin',
    min_patterns_leaf=args.min_patterns_leaf)

# b.build_tree_file(
                #  tree_path=f'filters/tree_{args.max_cube}_{args.min_patterns_leaf}.bin',
                #  min_patterns_leaf=args.min_patterns_leaf)


# for i in (1,5,10,15,20,25,30):
#     for j in range(20,81,5):
#         print(f'sh build_pybind.sh && python3 build_tree.py --max-cube {j} --min-patterns-leaf {i}')
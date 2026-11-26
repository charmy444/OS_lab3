#!/usr/bin/env python3
import argparse
from pathlib import Path
import random
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--binary', default='build/timsort')
args = parser.parse_args()
binary = str(Path(args.binary).resolve())
rng = random.Random(42)
cases = [[], [1], [2, 1], [7] * 257, [-2147483648, 2147483647, 0, -1, 0]]
for size in [31, 32, 33, 63, 64, 65, 127, 128, 129, 255, 511, 1024, 4097]:
    cases.extend([list(range(size)), list(range(size, 0, -1)),
                  [rng.randint(-10000, 10000) for _ in range(size)],
                  [rng.randrange(4) for _ in range(size)]])
runs = []
for length in [120, 80, 25, 20, 30, 16, 300, 70, 150, 45, 200]:
    start = rng.randrange(-5000, 5000)
    runs.extend(range(start, start + length))
cases.append(runs)
for _ in range(30):
    cases.append([rng.randrange(-200, 200) for _ in range(rng.randrange(3000))])
cases.extend([
    list(range(200, 240)) + list(range(0, 40)),
    [value for block in range(10, 0, -1) for value in range(block * 80, block * 80 + 65)],
    [8, 6, 4, 2, 0, 7, 5, 3, 1, -1, -3, -5, -7],
])
checks = 0
for values in cases:
    data = f'{len(values)}\n' + ' '.join(map(str, values)) + '\n'
    for threads in [1, 2, 3, 4, 7]:
        result = subprocess.run([binary, '--threads', str(threads)], input=data,
                                text=True, capture_output=True, timeout=20)
        assert result.returncode == 0, result.stderr
        actual = list(map(int, result.stdout.split()))
        assert actual == sorted(values), (threads, values[:30], actual[:30])
        assert re.search(r'Время сортировки: [0-9]+\.[0-9]{9} с\. Массив отсортирован\.', result.stderr)
        checks += 1
for data, expected in [
    ('  4\r\n +4\t-9 0\v-9', [-9, -9, 0, 4]),
    ('2\n' + '0' * 126 + '7 -3', [-3, 7]),
    ('0', []),
]:
    result = subprocess.run([binary, '--threads', '8'], input=data, text=True,
                            capture_output=True, timeout=10)
    assert result.returncode == 0, result.stderr
    assert list(map(int, result.stdout.split())) == expected, result.stdout
    checks += 1
for arguments, data in [([], ''), (['--threads', '0'], ''),
    (['--threads', '-1'], ''), (['--threads', '2', '--size', '5'], ''),
    (['--threads', '2'], '3 1 2'), (['--threads', '2'], '1 2147483648'),
    (['--threads', '2'], '1 -2147483649'), (['--threads', '2'], '-1'),
    (['--threads', '2'], '1 abc'), (['--threads', '2'], '1 12x'),
    (['--threads', '2'], '1 ' + '0' * 128),
    (['--threads', '2'], '0' * 128),
    (['--threads', '2'], '1 12\x00x'),
    (['--threads', '2'], '1\x00 12'),
    (['--threads', '2'], '1 -'),
    (['--threads', '2'], '2147483648'),
    (['--threads', '+2'], '0'),
    (['--threads', '18446744073709551616'], '')]:
    result = subprocess.run([binary, *arguments], input=data, text=True,
                            capture_output=True, timeout=10)
    assert result.returncode != 0, (arguments, data)
    assert result.stdout == '', result.stdout
    checks += 1
print(f'Пройдено проверок: {checks}. Результаты сравнены с Python sorted, ошибки ввода проверены.')

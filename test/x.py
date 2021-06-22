from funcy import keep
from neal import SimulatedAnnealingSampler
from cpp_pyquboc import Binary, Constraint, Placeholder
from timeit import timeit


M = 5   # 社員の数
D = 10  # 日数

BETA_RANGE = (5, 100)          # 焼きなましの温度の逆数。大きい方が解が安定しますが、局所解に陥る可能性が高くなってしまいます
NUM_READS = 10                 # 焼きなましする回数。NUM_READS個の解が生成されます。多いほうが良い解がでる可能性が高くなります
NUM_SWEEPS = 100000            # 焼きなましのステップを実施する回数。1つの解を生成するために繰り返し処理をする回数です。大きい方が良い解がでる可能性が高くなります
BETA_SCHEDULE_TYPE = 'linear'  # 焼きなましの温度をどのように変化させるか。linearだと線形に変化させます

# QUBOを構成する変数を定義します
xs = [[Binary(f'x[{i}][{j}]') for j in range(D)] for i in range(M)]

# チューニングのための変数を定義します
a = Placeholder('A')
b = Placeholder('B')

# QUBOを定義します。ここから……
h = 0

# 1日に2名以上、かつ、できるだけ少なくという制約を追加します。2名より多くても少なくてもペナルティが発生するようになっています
for d in range(D):
    h += a * Constraint((sum(xs[m][d] for m in range(M)) - 2) ** 2, f'member-count-{d}', lambda x: x == 0.0)  # 2を引くと、少なければ負、多ければ正の数になるわけですが、それを2乗して正の値にします

# 同じ人と別の日に出社しないという制約を追加します
for m1 in range(M):
    for m2 in range(m1 + 1, M):
        for d1 in range(D):
            for d2 in range(d1 + 1, D):
                h += b * Constraint(xs[m1][d1] * xs[m2][d1] * xs[m1][d2] * xs[m2][d2], f'different-member-{m1}-{m2}-{d1}-{d2}', lambda x: x == 0.0)  # xsは1か0なので、掛け算をする場合は、全部1の場合にだけ1になります

print(timeit('h.compile()', number=1000, globals=globals()) / 1000)

# コンパイルしてモデルを作ります
model = h.compile()
# ……ここまで。QUBOを定義します

# チューニングのための変数の値
feed_dict = {'A': 2.0, 'B': 1.0}

# イジング模型を生成して、nealで解きます
bqm = model.to_bqm(feed_dict=feed_dict)

samples = SimulatedAnnealingSampler().sample(bqm, beta_range=BETA_RANGE, num_reads=NUM_READS, num_sweeps=NUM_SWEEPS, beta_schedule_type=BETA_SCHEDULE_TYPE, seed=1)

answer = min(model.decode_sampleset(samples, feed_dict=feed_dict), key=lambda sample: sample.energy)

# 結果を出力します
print(f'broken:\t{answer.constraints(only_broken=True)}')  # Constraintに違反した場合は、brokenに値が入ります
print(f'energy:\t{answer.energy}')                         # QUBOのエネルギー。今回のモデルでは、全ての制約を満たした場合は0になります

# 日単位で、出社する社員を出力します
for d in range(D):
    print(tuple(keep(lambda m: 'ABCDE'[m] if answer.sample[f'x[{m}][{d}]'] == 1 else False, range(M))))

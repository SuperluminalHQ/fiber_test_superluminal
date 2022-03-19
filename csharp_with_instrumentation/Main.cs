using System;
using System.Threading.Tasks;

namespace MyApp
{
    class Program
    {
        static int Workload(int timeMs)
        {
            using (SuperluminalPerf.BeginEvent("Workload", $"timeMs = {timeMs}"))
            {
                int load = timeMs * 1000 * 1000;

                int result = 0;
                for (int i = 0; i < load; i++)
                {
                    result += i;
                }

                return result;
            }
        }

        static int InnerTestA(int timeMs)
        {
            return Workload(timeMs);
        }
        static int InnerTestB(int timeMs)
        {
            int load = 1000 * 1000;

            int result = 0;
            for (int i = 0; i < load; i++)
            {
                result += i;
            }
            return result + Workload(timeMs * 2);
        }
        static int InnerTest(int timeMs)
        {
            int rA = InnerTestA(timeMs / 2);
            int rB = InnerTestB(timeMs / 2);

            int load = timeMs * 1000 * 1000;

            int result = 0;
            for (int i = 0; i < load; i++)
            {
                result += i;
            }
            return result + rA + rB;
        }

        static void Test(int N)
        {
            using (SuperluminalPerf.BeginEvent("Test"))
            {
                int r0 = Workload(N);
                r0 += InnerTest(2 * N);
                Console.WriteLine($"result = {r0}");
            }
        }

        static Task<int> WorkloadAsync(int timeMs)
        {
            return Task.Run(() =>
            {
                return Workload(timeMs);
            });
        }

        static async Task<int> TestParallelWorkloadAsync(int N)
        {
            using (SuperluminalPerf.BeginEvent("TestParallelWorkloadAsync", $"N = {N}"))
            {
                var r0 = WorkloadAsync(650 + N);
                var r1 = WorkloadAsync(300 + 2 * N);
                var r2 = WorkloadAsync(700 + N);
                var r3 = WorkloadAsync(500 + N);

                var r = await r0 + await r1 + await r2 + await r3;

                return r;
            }
        }
        static async Task<int> TestWorkloadAsync(int N)
        {
            using (SuperluminalPerf.BeginEvent("TestWorkloadAsync", $"N = {N}"))
            {
                var r0 = await WorkloadAsync(650 + N);
                await Task.Delay(1000);
                var r1 = await WorkloadAsync(300 + 3 * N);

                var r = r0 + r1;

                return r;
            }
        }
        static async Task TestAsync()
        {
            using (SuperluminalPerf.BeginEvent("TestAsync"))
            {
                int r0 = await TestParallelWorkloadAsync(53);
                int r1 = await TestParallelWorkloadAsync(72);

                Console.WriteLine($"Async result = {r0 + r1}");
            }
        }
        static void TestTask()
        {
            Task.Run(Program.TestAsync).Wait();
        }

        [STAThread()]
        public static void Main()
        {
            SuperluminalPerf.Initialize();
            Test(1000);
            Test(300);
            Test(1500);

            TestTask();
        }
    }

}

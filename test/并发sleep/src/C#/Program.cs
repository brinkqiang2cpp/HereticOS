using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;

namespace ConsoleTest
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("按任意键开始测试 WebAPI:http://localhost:62219/api/values?sleepTime={int}");
            while (Test()) ;

            Console.ReadLine();
            Console.ReadLine();
        }

        private static bool Test()
        {
            Console.Write("请输入此API方法的睡眠时间（毫秒）,输入非数字内容退出：");
            string input = Console.ReadLine();
            int SleepTime = 50;
            if (!int.TryParse(input, out SleepTime))
                return false;
            HttpClient client = new HttpClient();
            client.BaseAddress = new Uri("http://localhost:62219/");
            var result = client.GetStringAsync("api/values?sleepTime=" + input).Result;
            Console.WriteLine("Result:{0}", result);
            int TaskNumber = 100;


            Console.WriteLine("{0}次 AIO（同步）测试(睡眠{1} 毫秒)：", TaskNumber, SleepTime);
            System.Diagnostics.Stopwatch sw = new System.Diagnostics.Stopwatch();


            sw.Start();
            Task[] taskArr = new Task[TaskNumber];
            for (int i = 0; i < TaskNumber; i++)
            {
                Task task = client.GetStringAsync("api/values?sleepTime2=" + SleepTime);
                taskArr[i] = task;

            }
            Task.WaitAll(taskArr);
            sw.Stop();
            double useTime1 = sw.Elapsed.TotalSeconds;
            Console.WriteLine("耗时（秒）：{0}", useTime1);
            sw.Reset();

            Console.WriteLine("{0}次 BIO（异步）测试(睡眠{1} 毫秒)：", TaskNumber, SleepTime);
            sw.Start();
            for (int i = 0; i < TaskNumber; i++)
            {
                Task task = client.GetStringAsync("api/values?sleepTime=" + SleepTime);
                taskArr[i] = task;
            }
            Task.WaitAll(taskArr);
            sw.Stop();
            double useTime2 = sw.Elapsed.TotalSeconds;
            Console.WriteLine("耗时（秒）：{0}", useTime2);
            return true;
        }
    }
}

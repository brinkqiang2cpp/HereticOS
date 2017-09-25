
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Created by Duan Rui on 2017/9/21 0021.
 */
public class TestIOPS
{
    private static int Task_Num = 10000;
    private static int Sleep_Period = 10000;
    private static int Pool_Size = 20000;

    private static CountDownLatch latch;
    private static ExecutorService service = Executors.newFixedThreadPool(Pool_Size);
    public static void main(String[] args)
    {
        latch = new CountDownLatch(Task_Num);
        Runnable[] tasks = new IOTask[Task_Num];
        for(int i=0; i< Task_Num; i++)
        {
            tasks[i] = new IOTask();
        }
        long start = System.currentTimeMillis();
        for(int i=0; i<Task_Num; i++)
        {
            service.execute(tasks[i]);
        }
        try
        {
            latch.await();
        }catch(InterruptedException e)
        {
            e.printStackTrace();
        }
        long time = System.currentTimeMillis() - start;
        System.out.println(String.format("Task num : %d, sleep period : %d, pool size : %d, time used : %d(ms).",
                            Task_Num, Sleep_Period, Pool_Size, time));
        System.out.println(String.format("iops : %.2f", ((Task_Num*1000.f)/time)));
        service.shutdown();
    }

    static class IOTask implements Runnable
    {
        @Override
        public void run()
        {
            try
            {
                Thread.sleep(Sleep_Period);
                latch.countDown();
            }catch(Exception e)
            {
                e.printStackTrace();
            }
        }
    }
}
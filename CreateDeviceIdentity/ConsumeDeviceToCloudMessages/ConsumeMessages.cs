using Microsoft.ServiceBus.Messaging;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ConsumeDeviceToCloudMessages
{
    class ConsumeMessages : IEventProcessor
    {

        Stopwatch checkpointStopWatch;

        async Task IEventProcessor.CloseAsync(PartitionContext context, CloseReason reason)
        {
            Console.WriteLine(string.Format("Processor Shuting Down.  Partition '{0}', Reason: '{1}'.", context.Lease.PartitionId, reason.ToString()));
            if (reason == CloseReason.Shutdown)
            {
                WriteHighlightedMessage(String.Format("Checkpointed partition: {0}", context.Lease.PartitionId), ConsoleColor.Yellow);

                await context.CheckpointAsync();
                lock (this)
                {
                    this.checkpointStopWatch.Restart();
                }
            }
        }

        Task IEventProcessor.OpenAsync(PartitionContext context)
        {
            Console.WriteLine(string.Format("ConsumeMessages initialize.  Partition: '{0}', Offset: '{1}'", context.Lease.PartitionId, context.Lease.Offset));
            this.checkpointStopWatch = new Stopwatch();
            this.checkpointStopWatch.Start();
            Console.WriteLine(".");
            return Task.FromResult<object>(null);
            
        }

        static int counter = 0;
        int nMessgaeSize = 80; 
        async Task IEventProcessor.ProcessEventsAsync(PartitionContext context, IEnumerable<EventData> messages)
        {
            foreach (EventData eventData in messages)
            {
                string data = Encoding.UTF8.GetString(eventData.GetBytes());
                if(counter > nMessgaeSize)
                {
                    Console.WriteLine("");
                    counter = 0;

                    WriteHighlightedMessage(String.Format("Checkpointed partition: {0}", context.Lease.PartitionId), ConsoleColor.Yellow);

                    await context.CheckpointAsync();
                    lock (this)
                    {
                        this.checkpointStopWatch.Restart();
                    }
                }
                counter++; 
                Console.Write(string.Format("."));
                //Console.WriteLine(string.Format("Message received.  Partition: '{0}', Data: '{1}'",
                //    context.Lease.PartitionId, data));
            }
        }

        private void WriteHighlightedMessage(string message, ConsoleColor color)
        {
            Console.ForegroundColor = color;
            Console.WriteLine(message);
            Console.ResetColor();
        }

    }
}

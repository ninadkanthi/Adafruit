using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.ServiceBus.Messaging;


namespace ConsumeDeviceToCloudMessages
{
    class Program
    {
        static void Main(string[] args)
        {
            string iotHubConnectionString = "[Insert your Key]";
            string eventProcessorHostName = Guid.NewGuid().ToString();
            //string eventHubName = "nkarduino";
            string storageAccountName = "nkimages";
            string storageAccountKey = "[Insert your Key]";
            string storageConnectionString = string.Format("DefaultEndpointsProtocol=https;AccountName={0};AccountKey={1}", storageAccountName, storageAccountKey);





           
            string iotHubD2cEndpoint = "messages/events";
          
            


            EventProcessorHost eventProcessorHost = new EventProcessorHost(eventProcessorHostName, iotHubD2cEndpoint, EventHubConsumerGroup.DefaultGroupName, iotHubConnectionString, storageConnectionString, "messages-events");
            Console.WriteLine("Registering EventProcessor...");
            eventProcessorHost.RegisterEventProcessorAsync<ConsumeMessages>().Wait();

            Console.WriteLine("Receiving. Press enter key to stop worker.");
            Console.ReadLine();
            eventProcessorHost.UnregisterEventProcessorAsync().Wait();





            //string strConsumerGroup = string.Empty;
            //if (args != null && args.Length > 0)
            //{
            //    strConsumerGroup = args[0];
            //}
            //EventProcessorHost eventProcessorHost;
            //if (strConsumerGroup.Equals(string.Empty))
            //{
            //    eventProcessorHost = new EventProcessorHost(eventProcessorHostName, iotHubD2cEndpoint, EventHubConsumerGroup.DefaultGroupName, iotHubConnectionString, StorageConnectionString, "messages-events");
            //}
            //else
            //{
            //    eventProcessorHost = new EventProcessorHost(eventProcessorHostName, iotHubD2cEndpoint, strConsumerGroup, iotHubConnectionString, StorageConnectionString, "messages-events");
            //}

        }
    }
}

using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.ExplorerAPI.Models
{
    public class TransactionData
    {
        [JsonProperty(PropertyName = "address")]
        public string Address
        {
            get; set;
        }

        [JsonProperty(PropertyName = "amount")]
        public string Amount
        {
            get; set;
        }

        [JsonProperty(PropertyName = "direction")]
        public string Direction
        {
            get; set;
        }

        [JsonProperty(PropertyName = "time")]
        public string TimeStamp
        {
            get; set;
        }
    }
}

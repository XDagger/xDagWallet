using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.Rpc.Models.Responses
{
    public class TransactionsResult : ModelBase
    {
        [JsonProperty(PropertyName = "total")]
        public int Total
        {
            get; set;
        }

        [JsonProperty(PropertyName = "transactions")]
        public List<TransactionResult> Transactions
        {
            get; set;
        }
    }

    public class TransactionResult : ModelBase
    {
        [JsonProperty(PropertyName = "state")]
        public string State
        {
            get; set;
        }

        [JsonProperty(PropertyName = "direction")]
        public string Direction
        {
            get; set;
        }

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

        [JsonProperty(PropertyName = "timestamp")]
        public string TimeStamp
        {
            get; set;
        }

        [JsonProperty(PropertyName = "remark")]
        public string Remark
        {
            get; set;
        }
    }
}

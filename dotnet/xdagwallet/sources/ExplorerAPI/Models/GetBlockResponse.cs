using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.ExplorerAPI.Models
{
    public class GetBlockResponse
    {
        public GetBlockResponse()
        {

        }

        [JsonProperty(PropertyName = "balance")]
        public string Balance
        {
            get; set;
        }

        [JsonProperty(PropertyName = "state")]
        public string State
        {
            get; set;
        }

        [JsonProperty(PropertyName = "block_as_address")]
        public List<TransactionData> TransactionsList
        {
            get; set;
        }

        [JsonProperty(PropertyName = "block_as_transaction")]
        public List<TransactionData> PartnersList
        {
            get; set;
        }
    }
}

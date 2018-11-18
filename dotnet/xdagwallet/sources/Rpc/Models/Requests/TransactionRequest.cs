using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.Rpc.Models.Requests
{
    public class TransactionRequest : ModelBase
    {
        public TransactionRequest()
        {
            this.Address = string.Empty;
            this.Page = 0;
            this.PageSize = 50;
        }

        public TransactionRequest(string address)
        {
            this.Address = address;
            this.Page = 0;
            this.PageSize = 50;
        }

        public TransactionRequest(string address, int page, int pagesize)
        {
            this.Address = address;
            this.Page = page;
            this.PageSize = pagesize;
        }
        
        [JsonProperty(PropertyName = "address")]
        public string Address
        {
            get; set;
        }

        [JsonProperty(PropertyName = "page")]
        public int Page
        {
            get; set;
        }

        [JsonProperty(PropertyName = "pagesize")]
        public int PageSize
        {
            get; set;
        }
    }
}

using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using XDagNetWallet.Rpc.Models.Requests;

namespace XDagNetWallet.Rpc.Methods
{
    public class GetTransactionsMethod : MethodBase
    {
        public GetTransactionsMethod(int identifier)
            : base(identifier, "xdag_get_transactions")
        {
        }

        public GetTransactionsMethod(int identifier, TransactionRequest request)
            : this(identifier)
        {
            this.Parameters.Add(request);
        }
    }
}

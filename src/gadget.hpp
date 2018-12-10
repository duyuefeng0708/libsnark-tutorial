#include <libsnark/gadgetlib1/gadgets/basic_gadgets.hpp>
#include <libsnark/gadgetlib1/gadgets/hashes/sha256/sha256_gadget.hpp>
#include <libff/algebra/fields/field_utils.hpp>
#include <libff/algebra/curves/mnt/mnt4/mnt4_init.hpp>
#include <libff/algebra/curves/mnt/mnt6/mnt6_init.hpp>
#include <libsnark/gadgetlib1/gadgets/curves/weierstrass_g1_gadget.hpp>
#include <libsnark/gadgetlib1/gadgets/curves/weierstrass_g2_gadget.hpp>
#include <libsnark/gadgetlib1/gadgets/pairing/pairing_checks.hpp>
#include <libsnark/gadgetlib1/gadgets/pairing/pairing_params.hpp>
#include <libsnark/gadgetlib1/gadgets/pairing/weierstrass_precomputation.hpp>

const int digest_size = 256;
bool sha256_padding[256] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0};

template<typename ppT>
class output_selector_gadget;

template<typename ppT>
class check_pairing_eq_gadget;

template<typename ppT>
class fair_auditing_gadget : public gadget<libff::Fr<ppT>> {
	
public:
	typedef libff::Fr<ppT> FieldT;
	
	
	// Primary input
	std::shared_ptr<G1_variable<ppT> > M;
	std::shared_ptr<G2_variable<ppT> > y;
	std::shared_ptr<G2_variable<ppT> > g; // TODO: embed this and y in circuit
	pb_variable_array<FieldT> alleged_digest;
	
	// Witness
	std::shared_ptr<G1_variable<ppT> > sigma;
	pb_variable_array<FieldT> r;
	
	// Gadgets
	std::shared_ptr<G1_checker_gadget<ppT> > check_M;
	std::shared_ptr<G2_checker_gadget<ppT> > check_y;
	std::shared_ptr<G2_checker_gadget<ppT> > check_g;
	std::shared_ptr<G1_checker_gadget<ppT> > check_sigma;
	
	
	std::shared_ptr<check_pairing_eq_gadget<ppT> > pairing_check;
	std::shared_ptr<output_selector_gadget<ppT> > selector;
	
	
	fair_auditing_gadget(protoboard<FieldT> &pb);
	void generate_r1cs_constraints();
	void generate_r1cs_witness(const libff::G1<other_curve<ppT> > &M_val,
														 const libff::G2<other_curve<ppT> > &y_val,
														 const libff::G2<other_curve<ppT> > &g_val,
														 const libff::bit_vector &alleged_digest_val,
														 const libff::G1<other_curve<ppT> > &sigma_val,
														 const libff::bit_vector &r_val);
	
	unsigned num_input_variables() const {
		return M->num_variables() + y->num_variables() + g->num_variables() + digest_size /* alleged_digest */ ;
	}
                               
};


template<typename ppT>
class output_selector_gadget : public gadget<libff::Fr<ppT>> {
public:
	typedef libff::Fr<ppT> FieldT;

	std::shared_ptr<sha256_compression_function_gadget<FieldT>> compute_sha_r;
	std::shared_ptr<digest_variable<FieldT>> sha_r, padding_var;
	std::shared_ptr<block_variable<FieldT>> block;
	
	const pb_variable<FieldT> t;
	const pb_variable_array<FieldT> r;
	
	pb_variable_array<FieldT> tmp1, tmp2, xor_r;
	
	pb_variable_array<FieldT> selected_digest;
 
	output_selector_gadget(protoboard<FieldT> &pb, const pb_variable<FieldT> &, const pb_variable_array<FieldT> &);
	void generate_r1cs_constraints();
    
	void generate_r1cs_witness();
	
	libff::bit_vector sha_padding() const
	{
			const unsigned num_zeros = 256 - 64;
			libff::bit_vector padding(num_zeros, false);
			// add 64 bit representation of 512 (our total block length)
			for (auto i = 1; i <= 64; i++) {
				padding.push_back(i == 54); // using the fact that [512]_b is 1 followed by 9 zeros
			}
			return padding;
	}
                               
};

template<typename ppT>
class check_pairing_eq_gadget : public gadget<libff::Fr<ppT>> {
public:
	typedef libff::Fr<ppT> FieldT;
	check_pairing_eq_gadget(protoboard<libff::Fr<ppT>> &pb,
													std::shared_ptr<G1_variable<ppT> > _a,
													std::shared_ptr<G2_variable<ppT> > _b,
													std::shared_ptr<G1_variable<ppT> > _c,
													std::shared_ptr<G2_variable<ppT> > _d);
	void generate_r1cs_constraints();
    
	//void generate_r1cs_witness(G1<other_curve<ppT> > _a, G2<other_curve<ppT> > _b, G1<other_curve<ppT> > _c, G2<other_curve<ppT> > _d);
	void generate_r1cs_witness();
	
	unsigned num_input_variables()
	{
		return a->num_variables() + b->num_variables() + c->num_variables() + d->num_variables();
	}

	// variables
	std::shared_ptr<G1_variable<ppT> > a;
	std::shared_ptr<G2_variable<ppT> > b;
	std::shared_ptr<G1_variable<ppT> > c;
	std::shared_ptr<G2_variable<ppT> > d;
	
	pb_variable<FieldT> is_valid;
    
	
	// values
	std::shared_ptr<G1_precomputation<ppT> > a_precomp;
	std::shared_ptr<G2_precomputation<ppT> > b_precomp;
	std::shared_ptr<G1_precomputation<ppT> > c_precomp;
	std::shared_ptr<G2_precomputation<ppT> > d_precomp;
	
	
	// gadgets
	std::shared_ptr<precompute_G1_gadget<ppT> > compute_a_precomp;
	std::shared_ptr<precompute_G2_gadget<ppT> > compute_b_precomp;
	std::shared_ptr<precompute_G1_gadget<ppT> > compute_c_precomp;
	std::shared_ptr<precompute_G2_gadget<ppT> > compute_d_precomp;
	
	std::shared_ptr<check_e_equals_e_gadget<ppT> > check_valid;

};

#include "gadget.tcc"

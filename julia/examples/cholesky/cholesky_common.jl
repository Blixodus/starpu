# Standard kernels for the Cholesky factorization
# U22 is the gemm update
# U21 is the trsm update
# U11 is the cholesky factorization

@target STARPU_CPU+STARPU_CUDA
@codelet function u11(sub11 :: Matrix{Float32}) :: Nothing
    nx :: Int32 = width(sub11)
    ld :: Int32 = ld(sub11)

    for z in 0:nx-1
        lambda11 :: Float32 = sqrt(sub11[z+1,z+1])
        sub11[z+1,z+1] = lambda11

        alpha ::Float32 = 1.0f0 / lambda11
        X :: Vector{Float32} = view(sub11, z+2:z+2+(nx-z-2), z+1)
        STARPU_SSCAL(nx-z-1, alpha, X, 1)

        alpha = -1.0f0
        A :: Matrix{Float32} = view(sub11, z+2:z+2+(nx-z-2), z+2:z+2+(nx-z-2))
	STARPU_SSYR("L", nx-z-1, alpha, X, 1, A, ld)
    end
    return
end

@target STARPU_CPU+STARPU_CUDA
@codelet function u21(sub11 :: Matrix{Float32},
                      sub21 :: Matrix{Float32}) :: Nothing
    ld11 :: Int32 = ld(sub11)
    ld21 :: Int32 = ld(sub21)
    nx21 :: Int32 = width(sub21)
    ny21 :: Int32 = height(sub21)
    alpha :: Float32 = 1.0f0
    STARPU_STRSM("R", "L", "T", "N", nx21, ny21, alpha, sub11, ld11, sub21, ld21)
    return
end

@target STARPU_CPU+STARPU_CUDA
@codelet function u22(left   :: Matrix{Float32},
                      right  :: Matrix{Float32},
                      center :: Matrix{Float32}) :: Nothing
    dx :: Int32 = width(center)
    dy :: Int32 = height(center)
    dz :: Int32 = width(left)
    ld21 :: Int32 = ld(left)
    ld12 :: Int32 = ld(center)
    ld22 :: Int32 = ld(right)
    alpha :: Float32 = -1.0f0
    beta :: Float32 = 1.0f0
    STARPU_SGEMM("N", "T", dy, dx, dz, alpha, left, ld21, right, ld12, beta, center, ld22)
    return
end

@inline function tag11(k)
    return starpu_tag_t((UInt64(1)<<60) | UInt64(k))
end

@inline function tag21(k, j)
    return starpu_tag_t((UInt64(3)<<60) | (UInt64(k)<<32) |  UInt64(j))
end

@inline function tag22(k, i, j)
    return starpu_tag_t((UInt64(4)<<60) | (UInt64(k)<<32) | (UInt64(i)<<16) |  UInt64(j))
end

function check(mat::Matrix{Float32})
    size_p = size(mat, 1)

    for i in 1:size_p
        for j in 1:size_p
            if j > i
                mat[i, j] = 0.0f0
            end
        end
    end

    test_mat ::Matrix{Float32} = zeros(Float32, size_p, size_p)

    syrk!('L', 'N', 1.0f0, mat, 0.0f0, test_mat)

    for i in 1:size_p
        for j in 1:size_p
            if j <= i
                orig = (1.0f0/(1.0f0+(i-1)+(j-1))) + ((i == j) ? 1.0f0*size_p : 0.0f0)
                err = abs(test_mat[i,j] - orig) / orig
                if err > 0.0001
                    got = test_mat[i,j]
                    expected = orig
                    error("[$i, $j] -> $got != $expected (err $err)")
                end
            end
        end
    end

    println(stderr, "Verification successful !")
end

function clean_tags(nblocks)
    for k in 1:nblocks
        starpu_tag_remove(tag11(k))

        for m in k+1:nblocks
            starpu_tag_remove(tag21(k, m))

            for n in k+1:nblocks
                if n <= m
                    starpu_tag_remove(tag22(k, m, n))
                end
            end
        end
    end
end

function main(size_p :: Int, nblocks :: Int; verify = false, verbose = false)
    mat :: Matrix{Float32} = zeros(Float32, size_p, size_p)

    # create a simple definite positive symetric matrix
    # Hilbert matrix h(i,j) = 1/(i+j+1)

    for i in 1:size_p
        for j in 1:size_p
            mat[i, j] = 1.0f0 / (1.0f0+(i-1)+(j-1)) + ((i == j) ? 1.0f0*size_p : 0.0f0)
        end
    end

    if verbose
        display(mat)
    end

    starpu_memory_pin(mat)

    t_start = time_ns()

    cholesky(mat, size_p, nblocks)

    t_end = time_ns()

    starpu_memory_unpin(mat)

    flop = (1.0*size_p*size_p*size_p)/3.0
    time_ms = (t_end-t_start) / 1e6
    gflops = flop/(time_ms*1000)/1000
    println("$size_p\t$time_ms\t$gflops")

    clean_tags(nblocks)

    if verbose
        display(mat)
    end

    if verify
        check(mat)
    end
end
